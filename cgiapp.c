#include "cgiapp.h"

int main(int argc, char **argv)
{
    int result = 0;
    FCGX_Request request;
    FCGX_Init();
    FCGX_InitRequest(&request, 0, 0);
    struct app* app = create_app(argc, argv);
    if (app->init) {
        result = app->init(app->data);
        if (!result) return result;
    }

    while(FCGX_Accept_r(&request) == 0) {
        result = app->process(app->data, &request);
        if (!result) break;
    }
    if (app->done) app->done(app->data);
    return result;
}
