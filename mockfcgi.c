#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#else
#define _vsnprintf vsnprintf
#endif
#include "mockfcgi.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static void stream_reserve(FCGX_Stream *stream, size_t n) {
    if (!stream->data) {
        stream->pos = stream->data = malloc(n);
        stream->length = n;
    }
    else {
        ptrdiff_t used;
        assert(stream->pos >= stream->data);
        used = stream->pos - stream->data;
        if (stream->length - used < n) {
            stream->length = used + n;
            stream->data = realloc(stream->data, stream->length);
            stream->pos = stream->data + used;
        }
    }
}

char *FCGX_GetParam(const char *name, FCGX_ParamArray envp) {
    int p;
    for (p = 0; envp->param[p]; p += 2) {
        if (strcmp(name, envp->param[p])==0) {
            return envp->param[p + 1];
        }
    }
    return 0;
}

int FCGX_FPrintF(FCGX_Stream *stream, const char *format, ...) {
    char buffer[4096]; // TODO: a wild size limit appears!
    int size;
    va_list argptr;
    va_start(argptr, format);
    assert(stream);
    assert(format);
    size = _vsnprintf(buffer, sizeof(buffer), format, argptr);
    if (size >= 0) {
        int n = FCGX_PutStr(buffer, size, stream);
        return n;
    }
    // ERROR
    return size;
}

int FCGX_PutStr(const char *str, int n, FCGX_Stream *stream) {
    size_t len = (size_t)n;
    assert(str);
    assert(n >= 0);
    assert(stream);
    stream_reserve(stream, len);
    memcpy(stream->pos, str, len);
    stream->pos += len;
    return n;
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

    if (*env) {
        int p;
        size_t len = strlen(env) + 1;
        req->envp = malloc(sizeof(struct FCGX_ParamArray));
        req->envp->paramstr = malloc(len);
        memcpy(req->envp->paramstr, env, len);
//        strcpy(req->envp->paramstr, env);
        tok = strtok(req->envp->paramstr, "=");
        for (p=0; tok && p != MAXPARAM;p+=2) {
            req->envp->param[p] = tok;
            if (tok) {
                tok = strtok(NULL, " ");
                req->envp->param[p+1] = tok;
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
