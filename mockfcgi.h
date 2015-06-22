#pragma once

#include <stddef.h>

#define MAXPARAM 8

typedef struct FCGX_ParamArray {
    char *paramstr;
    char *param[MAXPARAM];
} *FCGX_ParamArray;

typedef struct FCGX_Stream {
    size_t length;
    char *data, *pos;
}  FCGX_Stream;

typedef struct FCGX_Request {
    FCGX_ParamArray envp;
    FCGX_Stream *in;
    FCGX_Stream *out;
}  FCGX_Request;

char *FCGX_GetParam(const char *name, FCGX_ParamArray envp);
int FCGX_FPrintF(FCGX_Stream *stream, const char *format, ...);
int FCGX_PutStr(const char *str, int n, FCGX_Stream *stream);
int FCGX_GetStr(char *str, int n, FCGX_Stream *stream);

FCGX_Request *FCGM_CreateRequest(const char *body, const char *env);
FCGX_Stream *FCGM_CreateStream(const void *data, size_t size);
