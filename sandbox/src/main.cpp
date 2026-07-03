#include <iostream>

#include "core/application.h"

class SandboxApplication : public streak::Application {
public:
    void on_init() override {
        std::cout << "Sandbox Application Initialized" << std::endl;
    }

    void on_update() override {
        std::cout << "Sandbox Application Updating" << std::endl;
    }

    void on_exit() override {
        std::cout << "Sandbox Application Exiting" << std::endl;
    }

    virtual bool should_exit() const override {
        return true;
    }
};

streak::ApplicationPtr streak::create_application() {
    return std::make_unique<SandboxApplication>();
}