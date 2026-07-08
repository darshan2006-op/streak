#include <iostream>

#include "core/application.h"
#include "events/event_dispatcher.h"
#include "events/window_event.h"
#include "events/event.h"
#include "core/window.h"

bool handle_event(std::shared_ptr<streak::event::Event> event){
    switch (event->get_event_type())
    {
        case streak::event::WindowResizeEvent::event_type:{
            auto e = static_cast<streak::event::WindowResizeEvent*>(event.get());
            std::cout << e->get_width() << " " << e->get_height() << std::endl;
            break;
        }
        case streak::event::WindowCloseEvent::event_type:{
            std::cout << "Window closed" << std::endl;
            break;
        }
    }
    return true;
}

class SandboxApplication : public streak::Application {
public:
    void on_init() override {

        m_dispatcher.set_handler(handle_event);
        streak::WindowOptions options;
        options.width = 400;
        options.height = 400;
        options.title = "window";
        options.event_dispatcher = &m_dispatcher;

        window = streak::WindowSystem::get().create_window(options);

        std::cout << "Sandbox Application Initialized" << std::endl;
    }

    void on_update() override {

        m_dispatcher.drain();
    }

    void on_exit() override {
        std::cout << "Sandbox Application Exiting" << std::endl;
    }

    virtual bool should_exit() const override {
        return window->should_close();
    }
private:
    streak::Window* window;
    streak::event::EventDispatcher m_dispatcher;
};

streak::ApplicationPtr streak::create_application() {
    return std::make_unique<SandboxApplication>();
}