#pragma once

#ifdef MOCKFCGI
#include "mockfcgi.h"
#else
#include <fcgiapp.h>
#endif

typedef struct appdata {
    void *ptr;
} appdata;

typedef struct app {
    appdata data;
    int(*init)(appdata self);
    void(*done)(appdata self);
    int(*process)(appdata self, FCGX_Request* req);
} app;

struct app * create_app(int argc, char **argv);
