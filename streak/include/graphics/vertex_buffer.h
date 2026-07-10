#pragma once

namespace streak{
    class VertexBuffer{
        public:
            virtual ~VertexBuffer() = default;

            virtual void setData(const void* data, unsigned int size) = 0;
        protected:
            VertexBuffer() = default;
    };
}