#include "platform/opengl/opengl_renderer.h"

#include <iostream>
#include <condition_variable>
#include <map>

#include "glad/gl.h"

namespace streak{
    OpenGLRenderer::OpenGLRenderer(){

    }

    void OpenGLRenderer::init(Window* window){
        {
            std::unique_lock lock(m_data_mutex);
            m_context = GraphicsContext::create(GraphicsContextType::OpenGL);
            m_frame_semaphore = Semaphore::create(3);
            m_initialized.store(true, std::memory_order_release);
        }

        std::condition_variable ready_cv;
        std::mutex ready_mutex;
        bool ready = false;
        std::cout << "Starting OpenGL Renderer thread loop" << std::endl;

        m_render_thread = std::thread([this, window, &ready_cv, &ready_mutex, &ready](){
            {
                std::unique_lock lock(m_data_mutex);
                m_context->init(window);
            }

            bool notified_outer_thread = false;

            while (true)
            {
                if(!notified_outer_thread){
                    {
                        std::unique_lock lock(ready_mutex);
                        ready = true;
                    }
                    m_running.store(true, std::memory_order_release);
                    ready_cv.notify_one();
                    notified_outer_thread = true;
                }

                {
                    std::unique_lock lock(m_loop_mutex);
                    m_loop_cv.wait(lock, [this](){ return !m_task_queue.is_empty()
                        || !m_running.load(std::memory_order_acquire); 
                    });
                }

                if(!m_running.load(std::memory_order_acquire)){
                    break;
                }

                if (!m_task_queue.is_empty())
                {   
                    auto task = m_task_queue.pop();
                    task();
                }

                m_context->swap_buffers();
                m_frame_semaphore->release();
            }

            for (auto& program : m_object_cache.pipeline_program_cache)
            {
                glDeleteProgram(program.second);
            }

            for(auto& vao : m_object_cache.geometry_vao_cache)
            {
                glDeleteVertexArrays(1, &vao.second);
            }
            
            for(auto& vbo : m_object_cache.geometry_vbo_cache)
            {
                glDeleteBuffers(1, &vbo.second);
            }

            for(auto& ebo : m_object_cache.geometry_ebo_cache)
            {
                glDeleteBuffers(1, &ebo.second);
            }

            m_context->destroy();
        });

        {
            std::unique_lock lock(ready_mutex);
            ready_cv.wait(lock, [&ready](){ return ready; });
        }
        std::cout << "OpenGL Renderer thread loop Initialized " << std::endl;
    }

    void OpenGLRenderer::begin_frame(){
        m_frame_semaphore->acquire();
        {
            std::unique_lock lock(m_data_mutex);
            m_command_queue.push(RenderCommands{});
        }
    }

    void OpenGLRenderer::end_frame(){
        push_event([this](){
            RenderCommands commands;
            {
                std::unique_lock lock(m_data_mutex);
                try{

                    if(!m_command_queue.empty()){
                        commands = std::move(m_command_queue.front());
                        m_command_queue.pop();
                    }
                }
                catch(const std::exception& e){
                    std::cerr << "Error in end_frame: " << e.what() << std::endl;
                }
            }

            for(auto& command : commands.commands){
                command();
            }
        });
    }

    void OpenGLRenderer::resize(uint32_t width, uint32_t height){
        push_event([this, width, height](){
            m_context->resize(width, height);
        });
    }

    void OpenGLRenderer::clear(float r, float g, float b, float a){
        push_render_command([r, g, b, a](){
            glClearColor(r, g, b, a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        });
    }

    void OpenGLRenderer::destroy(){
        if (!m_initialized.load(std::memory_order_acquire))
        {
            return;
        }
        
        {
            std::unique_lock lock(m_data_mutex);
            m_running.store(false, std::memory_order_release);
            m_initialized.store(false, std::memory_order_release);
        }

        m_loop_cv.notify_one();
        if(m_render_thread.joinable()){
            m_render_thread.join();
        }
    }

    void OpenGLRenderer::push_event(const Task& task){
        m_task_queue.push(task);
        m_loop_cv.notify_one();
    }

    void OpenGLRenderer::push_render_command(const Command& command){
        std::unique_lock lock(m_data_mutex);
        if(!m_command_queue.empty()){
            m_command_queue.back().commands.push_back(command);
        }
    }

    static uint32_t compile_shader(GLenum shader_type, const std::string& source){
        uint32_t shader = glCreateShader(shader_type);
        const char* source_cstr = source.c_str();
        glShaderSource(shader, 1, &source_cstr, nullptr);
        glCompileShader(shader);

        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "Error compiling shader: " << infoLog << std::endl;
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    uint32_t OpenGLRenderer::create_program(const std::shared_ptr<Pipeline>& pipeline){
        uint32_t vs = compile_shader(GL_VERTEX_SHADER, pipeline->get_vertex_shader().c_str());
        uint32_t fs = compile_shader(GL_FRAGMENT_SHADER, pipeline->get_fragment_shader().c_str());

        uint32_t program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        int success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetProgramInfoLog(program, 1024, nullptr, infoLog);
            std::cerr << "Error linking program: " << infoLog << std::endl;
            glDeleteProgram(program);
            return 0;
        }
        glDeleteShader(vs);
        glDeleteShader(fs);
        return program;
    }

    static std::unordered_map<PrimitiveType, GLenum> primitive_type_map = {
        {PrimitiveType::Points, GL_POINTS},
        {PrimitiveType::Lines, GL_LINES},
        {PrimitiveType::Polygons, GL_TRIANGLES}
    };

    static std::unordered_map<PolygonMode, GLenum> polygon_mode_map = {
        {PolygonMode::Fill, GL_FILL},
        {PolygonMode::Line, GL_LINE},
        {PolygonMode::Point, GL_POINT}
    };

    static std::unordered_map<CullMode, GLenum> cull_mode_map = {
        {CullMode::None, GL_NONE},
        {CullMode::Front, GL_FRONT},
        {CullMode::Back, GL_BACK}
    };

    static void cache_primitive_config(const PrimitiveConfig& config, PrimitiveConfig& current_config){
        if (current_config.cull_mode != config.cull_mode)
        {
            glCullFace(cull_mode_map[config.cull_mode]);
            current_config.cull_mode = config.cull_mode;
        }

        if(current_config.polygon_mode != config.polygon_mode)
        {
            glPolygonMode(GL_FRONT_AND_BACK, polygon_mode_map[config.polygon_mode]);
            current_config.polygon_mode = config.polygon_mode;
        }

        if(current_config.line_width != config.line_width)
        {
            glLineWidth(config.line_width);
            current_config.line_width = config.line_width;
        }

        if(current_config.point_size != config.point_size)
        {
            glPointSize(config.point_size);
            current_config.point_size = config.point_size;
        }
    }

    static const std::unordered_map<VertexAttributeType, uint32_t> attribute_sizes = {
        {VertexAttributeType::Float3, 3 * sizeof(float)},
        {VertexAttributeType::Float2, 2 * sizeof(float)},
        {VertexAttributeType::Float4, 4 * sizeof(float)}
    };

    static const std::unordered_map<VertexAttributeType, uint32_t> attribute_component_counts = {
        {VertexAttributeType::Float3, 3},
        {VertexAttributeType::Float2, 2},
        {VertexAttributeType::Float4, 4}
    };

    static const std::unordered_map<VertexAttributeType, GLenum> attribute_type_map = {
        {VertexAttributeType::Float3, GL_FLOAT},
        {VertexAttributeType::Float2, GL_FLOAT},
        {VertexAttributeType::Float4, GL_FLOAT}
    };

    static void apply_vertex_format(const VertexFormat& vertex_format) {
        uint32_t stride = vertex_format.get_stride();
        uint32_t offset = 0;

        for (size_t i = 0; i < vertex_format.get_attribute_count(); ++i) {
            VertexAttributeType type = vertex_format.get_attribute(i);
            glEnableVertexAttribArray(i);
            glVertexAttribPointer(i, attribute_component_counts.at(type), attribute_type_map.at(type), GL_FALSE, stride, (void*)offset);
            offset += attribute_sizes.at(type);
        }
    }

    void OpenGLRenderer::draw(const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Geometry>& geometry) {
        push_render_command([this, &pipeline, &geometry](){
            auto& config = pipeline->get_primitive_config();
            auto& current_config = m_current_primitive_config;
            auto& program_cache = m_object_cache.pipeline_program_cache;
            auto& vao_cache = m_object_cache.geometry_vao_cache;
            auto& vbo_cache = m_object_cache.geometry_vbo_cache;
            auto& ebo_cache = m_object_cache.geometry_ebo_cache;

            // TODO: CLEAN THIS UP PROPERLY
            
            cache_primitive_config(config, current_config);
            
            uint32_t program_id;
            if (program_cache.find(pipeline) != program_cache.end())
            {
                program_id = program_cache.at(pipeline);
            }
            else
            {
                program_id = create_program(pipeline);
                program_cache.insert({pipeline, program_id});
            }

            if(vao_cache.find(geometry) == vao_cache.end() ||
               vbo_cache.find(geometry) == vbo_cache.end() ||
               ebo_cache.find(geometry) == ebo_cache.end())
            {
                add_geometry_to_cache(geometry, pipeline);
            }

            if (m_current_pipeline != pipeline)
            {
                glUseProgram(program_id);

                glBindVertexArray(vao_cache.at(geometry));

                glBindBuffer(GL_ARRAY_BUFFER, vbo_cache.at(geometry));
                apply_vertex_format(pipeline->get_vertex_format());

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_cache.at(geometry));

                m_current_pipeline = pipeline;
            }

            glDrawElements(primitive_type_map[config.type], geometry->get_indices().size(), GL_UNSIGNED_INT, nullptr);

        });
    }

    void OpenGLRenderer::add_geometry_to_cache(const std::shared_ptr<Geometry>& geometry, const std::shared_ptr<Pipeline>& pipeline) {
        uint32_t vao, vbo, ebo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        const auto& vertices = geometry->get_vertices();
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        const auto& indices = geometry->get_indices();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

        m_object_cache.geometry_vao_cache.insert({geometry, vao});
        m_object_cache.geometry_vbo_cache.insert({geometry, vbo});
        m_object_cache.geometry_ebo_cache.insert({geometry, ebo});
    }

    OpenGLRenderer::~OpenGLRenderer() {
        destroy();
    }
}