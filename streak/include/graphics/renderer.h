#pragma once

#include "graphics/graphics_context.h"

namespace streak{
    class Renderer{
        public:
            virtual ~Renderer() = default;
            Renderer(const Renderer&) = delete;
            Renderer& operator=(const Renderer&) = delete;

            virtual void resize(uint32_t width, uint32_t height) = 0;
            virtual void swap_buffers() = 0;
        protected:
            Renderer() = default;
    };
}