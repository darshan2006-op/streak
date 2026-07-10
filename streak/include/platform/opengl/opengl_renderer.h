#pragma once

#include "graphics/renderer.h"
#include "graphics/graphics_context.h"
#include "events/event_queue.h"
#include "core/counting_semaphore.h"

#include <atomic>
#include <condition_variable>
#include <thread>
#include <functional>

namespace streak{

    enum class RendererCommandType{
        Clear,
    };

    struct RendererCommand{
        RendererCommandType type;

        union{

            struct
            {
                float r;
                float g;
                float b;
                float a;
            } clear;
        };
        
    };
    
    class OpenGLRenderer : public Renderer{
        public:
            using RendererCommand = ::streak::RendererCommand;
            using RendererEvent = std::function<void()>;

            virtual ~OpenGLRenderer();
            OpenGLRenderer(const OpenGLRenderer&) = delete;
            OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;
            OpenGLRenderer();

            virtual void init(Window* context) override;
            virtual void begin_frame() override;
            virtual void end_frame() override;
            virtual void resize(uint32_t width, uint32_t height) override;
            virtual void clear(float r, float g, float b, float a) override;
            virtual void destroy() override;
        
        protected:
            void dispatch_command(const RendererCommand& command);
            void push_event(const std::shared_ptr<RendererEvent>& event);
            void push_command(RendererCommand command);
            std::atomic<bool> m_has_pending_frames;
            CountingSemaphore m_frame_in_flight_semaphore;
            GraphicsContextPtr m_context;
            event::EventQueue<RendererEvent> m_event_queue;
            std::vector<RendererCommand> m_recording_commands;
            std::vector<RendererCommand> m_pending_commands;
            std::condition_variable m_context_drain_cv;
            std::mutex m_context_drain_mutex;
            std::thread m_render_thread;
            std::atomic<bool> m_running;       
    };
}