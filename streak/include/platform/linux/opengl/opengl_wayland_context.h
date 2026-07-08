#pragma once

#include "graphics/graphics_context.h"

namespace streak{

    struct WaylandOpengGLContextData;
    struct WaylandOpenGLContextGlobals;

    class OpenGLWaylandContext : public GraphicsContext{
        public:
            OpenGLWaylandContext() = default;
            virtual ~OpenGLWaylandContext() = default;

            virtual GraphicsContextType get_type() const override { return GraphicsContextType::OpenGL; }
            virtual void init(const Window* window) override;
            virtual void resize(uint32_t width, uint32_t height) override;
            virtual void swap_buffers() override;
            virtual void destroy() override;
        protected:
            void cleanup();
            void make_current();

            Window* m_window;
            WaylandOpengGLContextData* m_context_data;
            static WaylandOpenGLContextGlobals* s_globals;
            static uint32_t s_instance_count;
    };
}