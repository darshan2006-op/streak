#include "platform/opengl/opengl_renderer.h"

#include <iostream>
#include <condition_variable>
#include <GL/gl.h>

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

    OpenGLRenderer::~OpenGLRenderer() {
        destroy();
    }
}