#include <iostream>

#include "core/application.h"
#include "core/window.h"

class SandboxApplication : public streak::Application {
public:
    void on_init() override {

        window = streak::WindowSystem::get().create_window({800, 600, "Sandbox Window"});
        std::cout << "Sandbox Application Initialized" << std::endl;
    }

    void on_update() override {

    }

    void on_exit() override {
        std::cout << "Sandbox Application Exiting" << std::endl;
    }

    virtual bool should_exit() const override {
        return window->should_close();
    }
private:
    streak::Window* window;
};

streak::ApplicationPtr streak::create_application() {
    return std::make_unique<SandboxApplication>();
}