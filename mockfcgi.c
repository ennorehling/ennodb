#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "mockfcgi.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

char *FCGX_GetParam(const char *name, FCGX_ParamArray envp) {
    int p;
    for (p = 0; envp->param[p]; p += 2) {
        if (strcmp(name, envp->param[p])) {
            return envp->param[p + 1];
        }
    }
    return 0;
}

int FCGX_FPrintF(FCGX_Stream *stream, const char *format, ...) {
    assert(stream);
    assert(format);
    return 0;
}

int FCGX_PutStr(const char *str, int n, FCGX_Stream *stream) {
    assert(str);
    assert(n >= 0);
    assert(stream);
    return 0;
}

int FCGX_GetStr(char *str, int n, FCGX_Stream *stream) {
    assert(str);
    assert(n >= 0);
    assert(stream);
    return 0;
}

FCGX_Stream *FCGM_CreateStream(const void *data, size_t size) {
    FCGX_Stream *strm = malloc(sizeof(FCGX_Stream));
    if (!strm) {
        return NULL;
    }
    if (!size) {
        strm->data = 0;
    }
    else {
        strm->data = malloc(size);
        if (!strm->data) {
            free(strm);
            return NULL;
        }
        memcpy(strm->data, data, size);
    }
    strm->pos = strm->data;
    strm->length = size;
    return strm;
}

FCGX_Request *FCGM_CreateRequest(const char *body, const char *env) {
    FCGX_Request *req = malloc(sizeof(FCGX_Request));
    char *tok;
    int p = 0;

    if (*env) {
        size_t len = strlen(env) + 1;
        req->envp = malloc(sizeof(FCGX_ParamArray));
        req->envp->paramstr = malloc(len);
        memcpy(req->envp->paramstr, env, len);
        tok = strtok(req->envp->paramstr, "=");
        while (tok && p != MAXPARAM) {
            req->envp->param[p++] = tok;
            if (tok) {
                tok = strtok(NULL, " ");
                req->envp->param[p++] = tok;
                assert(tok);
                tok = strtok(NULL, "=");
            }
        }
        if (p < MAXPARAM) {
            req->envp->param[p] = 0;
        }
    }
    else {
        req->envp = 0;
    }
    req->out = FCGM_CreateStream(NULL, 0);
    req->in = FCGM_CreateStream(body, strlen(body));

    return req;
}
