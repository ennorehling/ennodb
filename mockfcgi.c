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

char *FCGX_GetParam(const char *name, FCGX_ParamArray envp) {
    int p;
    for (p = 0; envp[p]; p += 2) {
        if (strcmp(name, envp[p]) == 0) {
            return envp[p + 1];
        }
    }
    return 0;
}

FCGX_Stream *FCGM_CreateStream(const void *data, size_t size) {
    FCGX_Stream *strm = malloc(sizeof(FCGX_Stream));
    if (!strm) {
        goto FCGM_CreateStream_fail;
    }
    strm->isReader = (size!=0);
    if (strm->isReader) {
        strm->data = malloc(size);
        if (!strm->data) {
            goto FCGM_CreateStream_fail;
        }
        memcpy(strm->data, data, size);
        strm->stop = strm->data + size;
        strm->wrNext = strm->stop;
        strm->rdNext = strm->data;
    }
    else {
        strm->wrNext = strm->rdNext = strm->stop = strm->data = 0;
    }
    return strm;
FCGM_CreateStream_fail:
    free(strm);
    return 0;
}

static void stream_reserve(FCGX_Stream *strm, size_t len) {
    unsigned char **next = strm->isReader ? &strm->rdNext : &strm->wrNext;
    assert(strm);
    if (*next + len > strm->stop) {
        unsigned char *buffer = (unsigned char *)strm->data;
        size_t size = *next - buffer;
        buffer = realloc(buffer, size + len);
        strm->stop = buffer + size + len;
        *next = buffer + size;
        strm->data = buffer;
    }
}

int FCGX_PutStr(const char *str, int n, FCGX_Stream *stream) {
    size_t len = (size_t)n;
    assert(str);
    assert(n >= 0);
    assert(stream);
    stream_reserve(stream, len);
    memcpy(stream->wrNext, str, len);
    stream->wrNext += len;
    return n;
}

int FCGX_GetStr(char *str, int n, FCGX_Stream *stream) {
    assert(str);
    assert(n >= 0);
    assert(stream);
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

FCGX_Request *FCGM_CreateRequest(const char *body, const char *env) {
    FCGX_Request *req = malloc(sizeof(FCGX_Request));
    char *tok;

    if (*env) {
        int p;
        size_t len = strlen(env) + 1;
        char *paramstr = malloc(len);

        req->envp = calloc(MAXPARAM+2, sizeof(char *));
        req->envp[MAXPARAM] = 0;
        req->envp[MAXPARAM+1] = paramstr;
        memcpy(paramstr, env, len);
        tok = strtok(paramstr, "=");
        for (p = 0; tok && p != MAXPARAM; p += 2) {
            req->envp[p] = tok;
            if (tok) {
                tok = strtok(NULL, " ");
                req->envp[p + 1] = tok;
                assert(tok);
                tok = strtok(NULL, "=");
            }
        }
        if (p < MAXPARAM) {
            req->envp[p] = 0;
        }
    }
    else {
        req->envp = 0;
    }
    req->out = FCGM_CreateStream(NULL, 0);
    req->in = FCGM_CreateStream(body, strlen(body));

    return req;
}
