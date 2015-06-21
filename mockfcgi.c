#include "mockfcgi.h"

#include <assert.h>

char *FCGX_GetParam(const char *name, FCGX_ParamArray envp) {
    assert(name);
    assert(envp);
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
