#pragma once
#include <fcgiapp.h>

typedef struct app {
    void *data;
    int (*init)(void *self);
    void (*done)(void *self);
    int (*process)(void *self, FCGX_Request* req);
} app;

struct app * create_app(int argc, char **argv);
