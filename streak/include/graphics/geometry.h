#pragma once

#include <cstdint>
#include <vector>
#include <cstring>
#include <memory>

namespace streak{
    class Geometry{
        public:
            virtual ~Geometry() = default;
            Geometry(const Geometry&) = delete;
            Geometry& operator=(const Geometry&) = delete;
            Geometry() = default;

            const std::vector<float>& get_vertices() {
                return m_vertices;
            }

            const std::vector<uint32_t>& get_indices() {
                return m_indices;
            }

            static std::shared_ptr<Geometry> from_raw(float* vertices, uint32_t* indices, size_t total_vertex_size, size_t vertex_size, size_t index_size);
        protected:
            std::vector<float> m_vertices;
            std::vector<uint32_t> m_indices;
            uint32_t m_indices_count = 0;
    };
}