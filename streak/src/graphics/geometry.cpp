#include "graphics/geometry.h"

namespace streak{
    std::shared_ptr<Geometry> Geometry::from_raw(float* vertices, uint32_t* indices, size_t total_vertex_size, size_t vertex_size, size_t index_size){
        auto geometry = std::make_shared<Geometry>();
        geometry->m_vertices.resize(total_vertex_size / sizeof(float));
        geometry->m_indices.resize(index_size / sizeof(uint32_t));

        std::memcpy(geometry->m_vertices.data(), vertices, total_vertex_size);
        std::memcpy(geometry->m_indices.data(), indices, index_size);

        geometry->m_indices_count = static_cast<uint32_t>(geometry->m_indices.size());

        return geometry;
    }
}