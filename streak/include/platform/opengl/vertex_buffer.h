#pragma once

#include "graphics/vertex_buffer.h"
#include "graphics/renderer.h"

namespace streak{
    class OpenGLVertexBuffer : public VertexBuffer{
        public:
            OpenGLVertexBuffer();
            ~OpenGLVertexBuffer() override;

            void setData(const void* data, unsigned int size) override;
        private:
            unsigned int m_buffer_id;
    };
}