#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace streak{
    enum class PrimitiveType : uint8_t {
        Points,
        Lines,
        Polygons
    };

    enum class PolygonMode : uint8_t{
        Fill,
        Line,
        Point
    };

    enum class CullMode : uint8_t{
        None,
        Front,
        Back
    };

    struct PrimitiveConfig{
        PrimitiveType type = PrimitiveType::Polygons;
        PolygonMode polygon_mode = PolygonMode::Fill;
        CullMode cull_mode = CullMode::Back;

        float line_width = 1.0f;
        float point_size = 1.0f;
    };
    
    struct SharedSource
    {
        std::string vertex_shader;
        std::string fragment_shader;
    };
    
    enum class VertexAttributeType : uint8_t{
        Float3,
        Float2,
        Float4,
    };

    class VertexFormat{
        public:
            VertexFormat() : m_stride(0) {}
            void add_attribute(VertexAttributeType type);

            VertexAttributeType get_attribute(size_t index) const {
                return m_attributes.at(index);
            }

            size_t get_attribute_count() const {
                return m_attributes.size();
            }

            uint32_t get_stride() const {
                return m_stride;
            }
        private:
            std::vector<VertexAttributeType> m_attributes;
            uint32_t m_stride;
    };

    class Pipeline{
        public:
            virtual ~Pipeline() = default;
            Pipeline(const Pipeline&) = delete;
            Pipeline& operator=(const Pipeline&) = delete;
            Pipeline() = default;
            
            const PrimitiveConfig& get_primitive_config() const {
                return m_primitive_config;
            }
            
            const std::string& get_vertex_shader() const {
                return m_shared_source.vertex_shader;
            }
            
            const std::string& get_fragment_shader() const {
                return m_shared_source.fragment_shader;
            }

            const VertexFormat& get_vertex_format() const {
                return m_vertex_format;
            }

            static std::shared_ptr<Pipeline> create(const PrimitiveConfig& config, const SharedSource& shader_source, const VertexFormat& vertex_format);
            
        private:
            PrimitiveConfig m_primitive_config;
            SharedSource m_shared_source;
            VertexFormat m_vertex_format;
    };
}