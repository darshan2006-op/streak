#include "graphics/renderer.h"

#include <stdexcept>
#include "platform/opengl/opengl_renderer.h"

namespace streak{
    RendererPtr Renderer::create(GraphicsContextType type) {
        switch (type) {
            case GraphicsContextType::OpenGL:
                return std::make_unique<OpenGLRenderer>();
            default:
                throw std::runtime_error("Unsupported graphics context type");
        }
    }
}