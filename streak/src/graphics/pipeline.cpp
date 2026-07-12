#include "graphics/pipeline.h"

#include <unordered_map>

namespace streak{

    static const std::unordered_map<VertexAttributeType, uint32_t> attribute_sizes = {
        {VertexAttributeType::Float3, 3 * sizeof(float)},
        {VertexAttributeType::Float2, 2 * sizeof(float)},
        {VertexAttributeType::Float4, 4 * sizeof(float)}
    };

    void VertexFormat::add_attribute(VertexAttributeType type){
        m_attributes.push_back(type);
        m_stride += attribute_sizes.at(type);
    }

    std::shared_ptr<Pipeline> Pipeline::create(const PrimitiveConfig& config, const SharedSource& shader_source, const VertexFormat& vertex_format) {
        std::shared_ptr<Pipeline> pipeline = std::make_shared<Pipeline>();
        pipeline->m_primitive_config = config;
        pipeline->m_shared_source = shader_source;
        pipeline->m_vertex_format = vertex_format;
        return pipeline;
    }
}