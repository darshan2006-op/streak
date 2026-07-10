#pragma once

#include "graphics/renderer.h"
#include "events/event_queue.h"
#include "mt_sync/semaphore.h"

#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

namespace streak{
    using Task = std::function<void()>; 
    using Command = std::function<void()>;
    
    using TaskQueue = event::EventQueue<Task>;
    using CommandList = std::vector<Command>;

    struct RenderCommands
    {
        CommandList commands;
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
            
            void end_frame() override;
        private:
            void push_render_command(const Command& command);
            void push_event(const Task& task);

            GraphicsContextPtr m_context;
            TaskQueue m_task_queue;
            std::queue<RenderCommands> m_command_queue;

            std::thread m_render_thread;
            std::mutex m_data_mutex;
            std::mutex m_loop_mutex;
            std::condition_variable m_loop_cv;
            std::unique_ptr<Semaphore> m_frame_semaphore;

            std::atomic<bool> m_running, m_initialized;
    };
}