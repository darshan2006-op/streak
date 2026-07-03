#include "core/application.h"

int main() {
    streak::ApplicationPtr app = streak::create_application();

    app->on_init();

    do{
        app->on_update();
    }while(!app->should_exit());

    app->on_exit();

    return 0;
}