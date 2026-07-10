#include "platform/opengl/opengl_renderer.h"

#include "GL/gl.h"

#include <condition_variable>
#include <iostream>

namespace streak{

    OpenGLRenderer::OpenGLRenderer(): m_context(nullptr), m_render_thread(), m_frame_in_flight_semaphore(2) {}

    void OpenGLRenderer::init(Window* window) {

        std::mutex ready_mutex;
        std::condition_variable ready_cv;
        bool ready = false;

        m_render_thread = std::thread([this, &ready_mutex, &ready_cv, &ready, window]() {
            this->m_context = GraphicsContext::create(GraphicsContextType::OpenGL);
            this->m_context->init(window);
            this->m_running.store(true, std::memory_order_release);
            m_has_pending_frames.store(false, std::memory_order_release);


            {
                std::lock_guard<std::mutex> lock(ready_mutex);
                ready = true;
            }
            
            ready_cv.notify_one();

            while (true)
            {
                {
                    std::unique_lock<std::mutex> lock(m_context_drain_mutex);
                    m_context_drain_cv.wait(lock, [this]() { return m_has_pending_frames.load(std::memory_order_acquire) || !m_event_queue.is_empty() || !m_running.load(std::memory_order_acquire); });
                }

                if (!m_running.load(std::memory_order_acquire)) {
                    break;
                }

                while (!m_event_queue.is_empty())
                {
                    auto event = m_event_queue.try_pop();
                    if(!event || !m_running.load(std::memory_order_acquire)) {
                        break;
                    }
                    (*event)();
                }
                
                {
                    std::lock_guard<std::mutex> lock(m_context_drain_mutex);
                    for(auto& command : m_pending_commands) {
                        dispatch_command(command);
                    }
                    m_pending_commands.clear();
                    m_has_pending_frames.store(false, std::memory_order_release);
                }

                m_context->swap_buffers();
                this->m_frame_in_flight_semaphore.release();

                if(!m_running.load(std::memory_order_acquire)) {
                    break;
                }
            }

            this->m_frame_in_flight_semaphore.release();

            m_context->destroy();
            m_context.reset();
            std::cout  << "Render thread exiting." << std::endl;
        });

        {
            std::unique_lock<std::mutex> lock(ready_mutex);
            ready_cv.wait(lock, [&ready]() { return ready; });
        }

        std::cout << "OpenGLRenderer initialized and render thread started." << std::endl;
    }

    void OpenGLRenderer::begin_frame() {
        if(!m_running.load(std::memory_order_acquire)) {
            return;
        }
    }
    
    void OpenGLRenderer::end_frame() {
        if(!m_running.load(std::memory_order_acquire)) {
            return;
        }
        m_frame_in_flight_semaphore.acquire();

        // No specific actions needed here for now.
        std::lock_guard<std::mutex> lock(m_context_drain_mutex);
        m_pending_commands = std::move(m_recording_commands);
        m_has_pending_frames.store(true, std::memory_order_release);
        m_context_drain_cv.notify_one();
        m_recording_commands.clear();
    }

    void OpenGLRenderer::resize(uint32_t width, uint32_t height) {
        push_event(std::make_shared<RendererEvent>([this, width, height]() {
            if (m_context) {
                m_context->resize(width, height);
            }
        }));
    }

    void OpenGLRenderer::clear(float r, float g, float b, float a) {
        push_command(RendererCommand{.type = RendererCommandType::Clear, .clear = {r, g, b, a}});
    }

    void OpenGLRenderer::dispatch_command(const RendererCommand& command) {
        switch (command.type) {
            case RendererCommandType::Clear:
                glClearColor(command.clear.r, command.clear.g, command.clear.b, command.clear.a);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                break;
            default:
                std::cerr << "Unknown renderer command type." << std::endl;
                break;
        }
    }

    void OpenGLRenderer::push_command(RendererCommand command) {
        std::lock_guard<std::mutex> lock(m_context_drain_mutex);
        if(!m_running.load(std::memory_order_acquire)) {
            return;
        }
        m_recording_commands.push_back(command);
    }

    void OpenGLRenderer::push_event(const std::shared_ptr<RendererEvent>& event) {
        std::lock_guard<std::mutex> lock(m_context_drain_mutex);
        if(!m_running.load(std::memory_order_acquire)) {
            return;
        }
        m_event_queue.push(event);
        m_context_drain_cv.notify_one();
    }

    OpenGLRenderer::~OpenGLRenderer() {
        destroy();
    }

    void OpenGLRenderer::destroy() {
        if(!m_running.load(std::memory_order_acquire)) {
            return;
        }

        m_running.store(false, std::memory_order_release);
        m_context_drain_cv.notify_all();
        std::cout << "Destroying OpenGLRenderer." << std::endl;
        
        if (m_render_thread.joinable()) {
            m_render_thread.join();
        }
        
        if (m_context) {
            m_context->destroy();
            m_context.reset();
        }

        std::cout << "OpenGLRenderer destroyed and render thread stopped." << std::endl;
    }
}