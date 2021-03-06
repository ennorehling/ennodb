#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#else
#define _snprintf snprintf
#endif

#define _min(a, b) (((a) < (b)) ? (a) : (b))

#include "cgiapp.h"
#include "nosql.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <critbit.h>
#include <iniparser.h>

#define VERSION "1.5"

static const char * binlog = NULL;
static const char *inifile = "ennodb.ini";
static dictionary *config;
static int readonly = 0;
static int cycle_log = 0;

static const char * get_prefix(const char *path) {
    const char * result = strrchr(path, '/');
    return result ? result+1 : 0;
}

static int http_response(FCGX_Stream *out, int http_code, const char *message, const char *body, size_t length)
{
    FCGX_FPrintF(out,
                 "Status: %d %s\r\n"
                 "Content-Type: text/plain\r\n"
                 "Content-Length: %u\r\n"
                 "\r\n", http_code, message, (unsigned int)length);
    if (body) {
        FCGX_PutStr(body, (int)length, out);
    }
    return http_code;
}

static int http_success(FCGX_Stream *out, db_entry *body) {
    size_t size = 0;
    const char * data = "";
    if (body) {
        data = (const char *)body->data;
        size = body->size;
    }
    return http_response(out, 200, "OK", data, size);
}

static int http_not_found(FCGX_Stream *out, const char *body) {
    size_t len = body ? strlen(body) : 0;
    return http_response(out, 404, "Not Found", body, len);
}

static int http_invalid_method(FCGX_Stream *out, const char *body) {
    size_t len = body ? strlen(body) : 0;
    return http_response(out, 405, "Invalid Method", body, len);
}

static void done(void *self) {
    db_table *pl = (db_table *)self;
    if (pl->binlog) {
        fclose(pl->binlog);
    }
    free(self);
}

static void signal_handler(int sig);

static int init(void * self)
{
    db_table *pl = (db_table *)self;
    if (!binlog) return 1;
    if (read_log(pl, binlog) < 0) {
        printf("could not read %s, aborting.\n", binlog);
        abort();
    }
    if (open_log(pl, binlog) != 0) {
        printf("could not open %s, aborting.\n", binlog);
        abort();
    }
    signal(SIGINT, signal_handler);
#ifdef SIGHUP
    signal(SIGHUP, signal_handler);
#endif
    return 0;
}

static int db_get(FCGX_Request *req, db_table *pl, const char *prefix) {
    db_entry entry;
    if (get_key(pl, prefix, &entry)==200) {
        http_success(req->out, &entry);
    }
    else {
        http_not_found(req->out, NULL);
    }
    return 0;
}

static int db_post(FCGX_Request *req, db_table *pl, const char *prefix) {
    char buffer[MAXENTRY];
    db_entry entry;
    size_t size = sizeof(buffer), len = 0;
    char *b;
    for (b = buffer; size; ) {
        int result = FCGX_GetStr(b, (int)size, req->in);
        if (result > 0) {
            size_t bytes = (size_t)result;
            b+=bytes;
            size-=bytes;
            len+=bytes;
        } else {
            break;
        }
    }
    entry.size = len;
    entry.data = malloc(len);
    memcpy(entry.data, buffer, len);
    set_key(pl, prefix, &entry);
    http_success(req->out, NULL);
    return 0;
}

static int db_dump_keys(FCGX_Request *req, db_table *pl, const char *key) {
    char body[4096]; // TODO: this limit is unnecessary.
    int total;
    struct db_cursor *cur;

    total = list_keys(pl, key, &cur);
    printf("found %d matches for prefix %s\n", total, key);
    if (total>0) {
        char *b = body;
        size_t len = sizeof(body) - 1;
        const char *key;
        db_entry *val;
        while (len && cursor_get(cur, &key, &val)) {
            int result = _snprintf(b, len, "%s: ", key);
            if (result > 0) {
                size_t bytes = _min(len - (size_t)result, val->size);
                strncpy(b + result, val->data, bytes);
                strncpy(b + result, val->data, bytes);
                bytes += (size_t)result;
                b += bytes;
                if (len>bytes) {
                    *b++ = '\n';
                    len = len - bytes - 1;
                }
            }
        }
        http_response(req->out, 200, "OK", body, strlen(body));
    } else {
        http_not_found(req->out, NULL);
    }
    return 0;
}

static int process(void *self, FCGX_Request *req)
{
    const char *script, *prefix, *method;
    db_table *pl = (db_table *)self;
    assert(self && req);

    if (cycle_log) {
        close_log(pl);
        open_log(pl, binlog);
        cycle_log = 0;
    }
    
    method = FCGX_GetParam("REQUEST_METHOD", req->envp);
    script = FCGX_GetParam("PATH_INFO", req->envp);
    prefix = get_prefix(script);

    printf("%s request for %s\n", method, prefix);

    if (!method || !prefix) {
        http_invalid_method(req->out, NULL);
        return -1;
    }
    
    if (strstr(script, "debug")) {
        // TODO: remove this HACK!
        char body[2048];
        _snprintf(body, sizeof(body), "script: %s\nmethod: %s\nprefix: %s\n", script, method, prefix);
        http_response(req->out, 200, "OK", body, strlen(body));
        return 0;
    }
    if (script[0]!='/' || script[2]!='/' || script+3!=prefix) {
        http_not_found(req->out, NULL);
        return 0;
    }
    switch (script[1]) {
    case 'k':
        if (strcmp(method, "GET")==0) {
            return db_get(req, pl, prefix);
        }
        else if (!readonly && strcmp(method, "POST")==0) {
            return db_post(req, pl, prefix);
        }
        else {
            http_invalid_method(req->out, NULL);
        }
        break;
    case 'l':
        return db_dump_keys(req, pl, prefix);
    default:
        break;
    }
    return 0;
}

static void reload_config(void) {
    dictionary *ini = iniparser_new(inifile);
    if (ini) {
        const char *str;
        readonly = iniparser_getint(ini, "ennodb:readonly", 0);
        str = iniparser_getstr(ini, "ennodb:database");
        if (str && (!binlog || strcmp(binlog, str)!=0)) {
            binlog = str;
            cycle_log = 1;
        }
        if (config) {
            iniparser_free(config);
        }
        config = ini;
    }
}

static struct app myapp = {
    0, init, done, process
};

static void signal_handler(int sig) {
    if (sig==SIGINT) {
        printf("received SIGINT\n");
        done(myapp.data);
        abort();
    }
#ifdef SIGHUP
    if (sig==SIGHUP) {
        printf("received SIGHUP\n");
        fflush(((db_table *)myapp.data)->binlog);
        reload_config();
    }
#endif
}

static void print_version(void) {
    printf("EnnoDB %s\nCopyright (C) 2015 Enno Rehling.\n", VERSION);
}

struct app * create_app(int argc, char **argv) {
    if (argc>1) {
        int i;
        for (i=1; i!=argc;++i) {
            if (argv[i][0]=='-') {
                char opt = argv[i][1];
                switch (opt) {
                case 'v':
                    print_version();
                    break;
                }
            }
            else {
                inifile = argv[i];
            }
        }
    }
    reload_config();
    myapp.data = calloc(1, sizeof(db_table));
    return &myapp;
}
