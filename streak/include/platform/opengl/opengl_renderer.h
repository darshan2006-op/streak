#pragma once

#include "graphics/renderer.h"
#include "events/event_queue.h"
#include "mt_sync/semaphore.h"
#include "graphics/pipeline.h"

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

#include <map>

namespace streak{
    using Task = std::function<void()>; 
    using Command = std::function<void()>;
    
    using TaskQueue = event::EventQueue<Task>;
    using CommandList = std::vector<Command>;

    struct RenderCommands
    {
        CommandList commands;
    };

    struct ObjectCache
    {
        std::unordered_map<std::shared_ptr<Pipeline>, uint32_t> pipeline_program_cache;

        std::unordered_map<std::shared_ptr<Geometry>, uint32_t> geometry_vao_cache;
        std::unordered_map<std::shared_ptr<Geometry>, uint32_t> geometry_vbo_cache;
        std::unordered_map<std::shared_ptr<Geometry>, uint32_t> geometry_ebo_cache;
    };
    

    class OpenGLRenderer : public Renderer {
        public:
            OpenGLRenderer();
            virtual ~OpenGLRenderer() ;

            void init(Window* context) override;
            void destroy() override;

            void begin_frame() override;
            
            void resize(uint32_t width, uint32_t height) override;
            void clear(float r, float g, float b, float a) override;
            void draw(const std::shared_ptr<Pipeline>& pipeline, const std::shared_ptr<Geometry>& geometry) override;

            void end_frame() override;
            void push_event(const Task& task);

            private:
            void push_render_command(const Command& command);

            uint32_t create_program(const std::shared_ptr<Pipeline>& pipeline);

            void add_geometry_to_cache(const std::shared_ptr<Geometry>& geometry, const std::shared_ptr<Pipeline>& pipeline);

            GraphicsContextPtr m_context;
            TaskQueue m_task_queue;
            std::queue<RenderCommands> m_command_queue;
            ObjectCache m_object_cache;

            PrimitiveConfig m_current_primitive_config;
            std::shared_ptr<Pipeline> m_current_pipeline;

            std::thread m_render_thread;
            std::mutex m_data_mutex;
            std::mutex m_loop_mutex;
            std::condition_variable m_loop_cv;
            std::unique_ptr<Semaphore> m_frame_semaphore;

            std::atomic<bool> m_running, m_initialized;
    };
}