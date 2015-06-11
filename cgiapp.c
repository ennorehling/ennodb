#include "cgiapp.h"

int main(int argc, char **argv)
{
    FCGX_Request request;
    FCGX_Init();
    FCGX_InitRequest(&request, 0, 0);
    struct app* app = create_app(argc, argv);
    if (app->init) app->init(app->data);

    while(FCGX_Accept_r(&request) == 0) {
        app->process(app->data, &request);
    }
    if (app->done) app->done(app->data);
    return 0;
}
