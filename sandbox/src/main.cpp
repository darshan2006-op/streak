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

        streak::WindowOptions options;
        options.width = 800;
        options.height = 600;
        options.title = "window";
        options.event_dispatcher = &m_dispatcher1;

        m_renderer1 = streak::Renderer::create(streak::GraphicsContextType::OpenGL);
        window1 = streak::WindowSystem::get().create_window(options);
        m_renderer1->init(window1);

        float vertices[] = {
            -0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.5f, 
             0.5f, -0.5f, 0.0f, 0.5f, 0.5f, 0.5f,
             0.5f,  0.5f, 0.0f, 0.5f, 0.5f, 0.5f,
            -0.5f,  0.5f, 0.0f, 0.5f, 0.5f, 0.5f,
        };

        uint32_t indices[] = {
            0, 1, 2,
            2, 3, 0
        };

        m_geometry = streak::Geometry::from_raw(
            vertices,
            indices,
            sizeof(vertices),
            6 * sizeof(float),
            sizeof(indices)
        );

        streak::PrimitiveConfig config;
        config.type = streak::PrimitiveType::Polygons;
        config.polygon_mode = streak::PolygonMode::Fill;
        config.cull_mode = streak::CullMode::Back;
        config.line_width = 1.0f;
        config.point_size = 1.0f;

        streak::SharedSource shader_source;
        shader_source.vertex_shader = R"(
            #version 330 core
            
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec3 aColor;
            out vec3 ourColor;

            void main()
            {
                gl_Position = vec4(aPos, 1.0);
                ourColor = aColor;
            }
        )";
        shader_source.fragment_shader = R"(
            #version 330 core

            in vec3 ourColor;
            out vec4 FragColor;
            
            void main()
            {
                FragColor = vec4(ourColor, 1.0);
            }
        )";

        streak::VertexFormat vertex_format;
        vertex_format.add_attribute(streak::VertexAttributeType::Float3);
        vertex_format.add_attribute(streak::VertexAttributeType::Float3);

        m_pipeline = streak::Pipeline::create(config, shader_source, vertex_format);

        shader_source.vertex_shader = R"(
            #version 330 core
            
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in vec3 aColor;

            void main()
            {
                gl_Position = vec4(aPos, 1.0);
            }
        )";

        shader_source.fragment_shader = R"(
            #version 330 core

            out vec4 FragColor;
            
            void main()
            {
                FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0);
            }
        )";

        config.polygon_mode = streak::PolygonMode::Line;
        config.line_width = 4.0f;
        m_line_pipeline = streak::Pipeline::create(config, shader_source, vertex_format);
        
        std::cout << "Sandbox Application Initialized" << std::endl;
    }
    
    void on_update() override {

        m_renderer1->begin_frame();
        m_renderer1->clear(0.1f, 0.1f, 0.1f, 1.0f);
        m_renderer1->draw(m_pipeline, m_geometry);
        m_renderer1->draw(m_line_pipeline, m_geometry);
        m_renderer1->end_frame();
        m_dispatcher1.drain();

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
private:
    streak::Window* window1;
    std::shared_ptr<streak::Pipeline> m_pipeline, m_line_pipeline;
    std::shared_ptr<streak::Geometry> m_geometry;
    streak::RendererPtr m_renderer1;
    streak::event::EventDispatcher m_dispatcher1, m_dispatcher2;
    bool m_should_exit = false;
};

streak::ApplicationPtr streak::create_application() {
    return std::make_unique<SandboxApplication>();
}