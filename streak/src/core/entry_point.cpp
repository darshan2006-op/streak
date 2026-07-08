#include "core/application.h"
#include "core/window.h"

int main() {

    streak::ApplicationPtr app = streak::create_application();

    streak::WindowSystem::get().init();

    app->on_init();

    do{
        app->on_update();
    }while(!app->should_exit());

    app->on_exit();

    streak::WindowSystem::get().destroy();
    return 0;
}