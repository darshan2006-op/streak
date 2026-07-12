#pragma once

#include "graphics/graphics_context.h"
#include "core/window.h"
#include "graphics/pipeline.h"
#include "graphics/geometry.h"

namespace streak{
    using RendererPtr = std::unique_ptr<class Renderer>;
    class Renderer{
        public:
            virtual ~Renderer() = default;
            Renderer(const Renderer&) = delete;
            Renderer& operator=(const Renderer&) = delete;

            static RendererPtr create(GraphicsContextType type);

            virtual void init(Window* context) = 0;
            virtual void destroy() = 0;

            virtual void begin_frame() = 0;
            
            virtual void resize(uint32_t width, uint32_t height) = 0;
            virtual void clear(float r, float g, float b, float a) = 0;
            virtual void draw(const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Geometry>& geometry) = 0;

            virtual void end_frame() = 0;
        protected:
            Renderer() = default;
    };
}