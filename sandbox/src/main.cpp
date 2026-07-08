#include <iostream>

#include "core/application.h"
#include "events/event_dispatcher.h"
#include "events/window_event.h"
#include "events/event.h"
#include "core/window.h"
#include "graphics/graphics_context.h"

#include <GL/gl.h>

class SandboxApplication : public streak::Application {
public:
    void on_init() override {

        m_dispatcher.set_handler(std::bind(&SandboxApplication::handle_event, this, std::placeholders::_1));

        streak::WindowOptions options;
        options.width = 800;
        options.height = 600;
        options.title = "window";
        options.event_dispatcher = &m_dispatcher;

        window = streak::WindowSystem::get().create_window(options);
        m_graphics_context = streak::GraphicsContext::create(streak::GraphicsContextType::OpenGL);

        std::cout << "Sandbox Application Initialized" << std::endl;
    }
    
    void on_update() override {
        
        m_graphics_context->swap_buffers();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex2f(-0.5f, -0.5f);
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex2f(0.5f, -0.5f);
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex2f(0.0f, 0.5f);
        glEnd();

        m_dispatcher.drain();
    }

    void on_exit() override {
        std::cout << "Sandbox Application Exiting" << std::endl;
    }

    virtual bool should_exit() const override {
        return m_should_exit;
    }

    bool handle_event(std::shared_ptr<streak::event::Event> event){
        switch (event->get_event_type())
        {
            case streak::event::WindowConfiguredEvent::event_type: {
                m_graphics_context->init(window);
                break;
            }
            case streak::event::WindowResizeEvent::event_type:{
                auto e = static_cast<streak::event::WindowResizeEvent*>(event.get());
                m_graphics_context->resize(e->get_width(), e->get_height());
                std::cout << e->get_width() << " " << e->get_height() << std::endl;
                break;
            }
            case streak::event::WindowCloseEvent::event_type:{
                m_graphics_context->destroy();
                std::cout << "Window closed" << std::endl;
                m_should_exit = true;
                break;
            }
        }
        return true;
    }
private:
    streak::Window* window;
    bool m_should_exit = false;
    streak::event::EventDispatcher m_dispatcher;
    streak::GraphicsContextPtr m_graphics_context;
};

streak::ApplicationPtr streak::create_application() {
    return std::make_unique<SandboxApplication>();
}