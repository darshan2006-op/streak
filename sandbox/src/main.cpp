#include <iostream>

#include "core/application.h"
#include "events/event_dispatcher.h"
#include "events/window_event.h"
#include "events/event.h"
#include "core/window.h"
#include "graphics/renderer.h"

#include <GL/gl.h>

class SandboxApplication : public streak::Application {
public:
    void on_init() override {

        m_dispatcher1.set_handler(std::bind(&SandboxApplication::handle_event1, this, std::placeholders::_1));
        m_dispatcher2.set_handler(std::bind(&SandboxApplication::handle_event2, this, std::placeholders::_1));

        streak::WindowOptions options;
        options.width = 800;
        options.height = 600;
        options.title = "window";
        options.event_dispatcher = &m_dispatcher1;

        m_renderer1 = streak::Renderer::create(streak::GraphicsContextType::OpenGL);
        window1 = streak::WindowSystem::get().create_window(options);
        m_renderer1->init(window1);

        options.width = 400;
        options.height = 300;
        options.event_dispatcher = &m_dispatcher2;
        
        m_renderer2 = streak::Renderer::create(streak::GraphicsContextType::OpenGL);
        window2 = streak::WindowSystem::get().create_window(options);
        m_renderer2->init(window2);

        std::cout << "Sandbox Application Initialized" << std::endl;
    }
    
    void on_update() override {

        m_renderer1->begin_frame();
        m_renderer1->clear(0.1f, 0.1f, 0.1f, 1.0f);
        m_renderer1->end_frame();
        m_dispatcher1.drain();

        m_renderer2->begin_frame();
        m_renderer2->clear(0.1f, 0.0f, 0.1f, 1.0f);
        m_renderer2->end_frame();
        m_dispatcher2.drain();
    }

    void on_exit() override {
        std::cout << "Sandbox Application Exiting" << std::endl;
    }

    virtual bool should_exit() const override {
        return m_should_exit;
    }

    bool handle_event1(std::shared_ptr<streak::event::Event> event){
        switch (event->get_event_type())
        {
            case streak::event::WindowResizeEvent::event_type:{
                auto e = static_cast<streak::event::WindowResizeEvent*>(event.get());
                std::cout << "resize event1: " << e->get_width() << " " << e->get_height() << std::endl;
                std::cout << e->get_width() << " " << e->get_height() << std::endl;
                break;
            }
            case streak::event::WindowCloseEvent::event_type:{
                std::cout << "Window1 closed" << std::endl;
                m_renderer1->destroy();
                m_should_exit = true;
                window1->notify_close();
                break;
            }
        }
        return true;
    }

    bool handle_event2(std::shared_ptr<streak::event::Event> event){
        switch (event->get_event_type())
        {
            case streak::event::WindowResizeEvent::event_type:{
                auto e = static_cast<streak::event::WindowResizeEvent*>(event.get());
                std::cout << "resize event2: " << e->get_width() << " " << e->get_height() << std::endl;
                m_renderer2->resize(e->get_width(), e->get_height());
                break;
            }
            case streak::event::WindowCloseEvent::event_type:{
                std::cout << "Window2 closed" << std::endl;
                m_renderer2->destroy();
                m_should_exit = true;
                window2->notify_close();
                break;
            }
        }
        return true;
    }
private:
    streak::Window* window1, *window2;
    bool m_should_exit = false;
    streak::event::EventDispatcher m_dispatcher1, m_dispatcher2;
    streak::RendererPtr m_renderer1, m_renderer2;
};

streak::ApplicationPtr streak::create_application() {
    return std::make_unique<SandboxApplication>();
}