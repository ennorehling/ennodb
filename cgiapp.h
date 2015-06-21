#pragma once

#ifdef MOCKFCGI
#include "mockfcgi.h"
#else
#include <fcgiapp.h>
#endif

typedef struct app {
    void *data;
    int (*init)(void *self);
    void (*done)(void *self);
    int (*process)(void *self, FCGX_Request* req);
} app;

struct app * create_app(int argc, char **argv);
