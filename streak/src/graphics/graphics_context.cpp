#include "graphics/graphics_context.h"

#include "platform/linux/opengl/opengl_wayland_context.h"
#include "core/platform.h"

#include <iostream>

namespace streak{
    GraphicsContextPtr GraphicsContext::create(GraphicsContextType type){
        auto platform = Platform::get_instance().get_platform_value();

        if (platform == PlatformValue::Linux)
        {
            switch (type)
            {
                case GraphicsContextType::OpenGL:
                    return std::make_unique<OpenGLWaylandContext>();
                default:
                    std::cerr << "Unsupported graphics context type for Linux" << std::endl;
                    exit(EXIT_FAILURE);
            }
        }
        
    }
}