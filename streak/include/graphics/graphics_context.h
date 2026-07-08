#pragma once

#include <memory>
#include "core/window.h"

namespace streak{
    enum class GraphicsContextType{
        Vulkan,
        OpenGL,
    };

    using GraphicsContextPtr = std::unique_ptr<class GraphicsContext>;

    class GraphicsContext{
        public:
            virtual ~GraphicsContext() = default;
            GraphicsContext(const GraphicsContext&) = delete;
            GraphicsContext& operator=(const GraphicsContext&) = delete;

            static GraphicsContextPtr create(GraphicsContextType type);

            virtual GraphicsContextType get_type() const = 0;
            virtual void init(const Window* window) = 0;
            virtual void resize(uint32_t width, uint32_t height) = 0;
            virtual void swap_buffers() = 0;
            virtual void destroy() = 0;
        protected:
            GraphicsContext() = default;

    };
}