#include "platform/linux/opengl/opengl_wayland_context.h"

#include <iostream>

#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>

#include "platform/linux/wayland_window.h"


namespace streak{
    
    WaylandOpenGLContextGlobals* OpenGLWaylandContext::s_globals = nullptr;
    uint32_t OpenGLWaylandContext::s_instance_count = 0;

    struct WaylandOpenGLContextGlobals
    {
        EGLDisplay egl_display;
        EGLConfig egl_config;
    };

    struct WaylandOpengGLContextData
    {
        wl_egl_window *egl_window;
        EGLSurface egl_surface;
        EGLContext egl_context;
    };
    
    static void initialize_wayland_opengl_globals(WaylandOpenGLContextGlobals* globals){
        auto wayland_globals = WaylandWindowSystem::get().get_globals();

        globals->egl_display = eglGetDisplay(wayland_globals->display);

        if (globals->egl_display == EGL_NO_DISPLAY) {
            std::cerr << "Failed to get EGL display" << std::endl;
            return;
        }

        if (!eglInitialize(globals->egl_display, nullptr, nullptr)) {
            std::cerr << "Failed to initialize EGL" << std::endl;
            return;
        }

        EGLint config_attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
            
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,

            EGL_DEPTH_SIZE, 24,
            EGL_STENCIL_SIZE, 8,

            EGL_NONE
        };

        EGLint num_configs;
        EGLBoolean result = eglChooseConfig(globals->egl_display, config_attribs, &globals->egl_config, 1, &num_configs);

        if (result == EGL_FALSE || num_configs == 0) {
            std::cerr << "Failed to choose EGL config" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    static void cleanup_wayland_opengl_globals(WaylandOpenGLContextGlobals* globals){
        if (globals->egl_display != EGL_NO_DISPLAY) {
            eglTerminate(globals->egl_display);
            globals->egl_display = EGL_NO_DISPLAY;
        }
    }

    static void initialize_wayland_opengl_context(WaylandOpengGLContextData* context_data, WaylandOpenGLContextGlobals* globals, WaylandWindowData* window_data){
        eglBindAPI(EGL_OPENGL_API);

        context_data->egl_window = wl_egl_window_create(window_data->surface, window_data->width, window_data->height);

        context_data->egl_surface = eglCreateWindowSurface(globals->egl_display, globals->egl_config, context_data->egl_window, nullptr);

        context_data->egl_context = eglCreateContext(globals->egl_display, globals->egl_config, EGL_NO_CONTEXT, nullptr);

        if (context_data->egl_context == EGL_NO_CONTEXT) {
            std::cerr << "Failed to create EGL context" << std::endl;
            exit(EXIT_FAILURE);
        }

        if (context_data->egl_surface == EGL_NO_SURFACE)
        {
            std::cerr << "Failed to create EGL window surface" << std::endl;
            exit(EXIT_FAILURE);
        }
        
        if(context_data->egl_window == nullptr){
            std::cerr << "Failed to create Wayland EGL window" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    void OpenGLWaylandContext::init(const Window* window){
        m_window = const_cast<Window*>(window); 

        if(s_globals == nullptr){
            s_globals = new WaylandOpenGLContextGlobals();
            initialize_wayland_opengl_globals(s_globals);
        }
        s_instance_count++;

        {
            bool context_ready = false;
            std::mutex context_ready_mutex;
            std::condition_variable context_ready_cv;

            WaylandWindowSystem::get().push_task([this, &context_ready, &context_ready_mutex, &context_ready_cv](){
                m_context_data = new WaylandOpengGLContextData();
                initialize_wayland_opengl_context(m_context_data, s_globals, static_cast<WaylandWindowData*>(m_window->get_native_window_data()));
                std::cout << "OpenGL context initialized for Wayland window" << std::endl;
                {
                    std::lock_guard<std::mutex> lock(context_ready_mutex);
                    context_ready = true;
                }

                context_ready_cv.notify_one();
            });

            {
                std::unique_lock<std::mutex> lock(context_ready_mutex);
                context_ready_cv.wait(lock, [&context_ready] { return context_ready; });
            }
        }
        eglBindAPI(EGL_OPENGL_API);
        make_current();
    }

    void OpenGLWaylandContext::make_current(){
        if (m_context_data == nullptr || m_context_data->egl_window == nullptr) return;
        if (!eglMakeCurrent(s_globals->egl_display, m_context_data->egl_surface, m_context_data->egl_surface, m_context_data->egl_context)) {
            std::cerr << "Failed to make EGL context current" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    void OpenGLWaylandContext::resize(uint32_t width, uint32_t height){
        if (m_context_data == nullptr || m_context_data->egl_window == nullptr) return;
        std::cout << "Resizing OpenGL context to " << width << "x" << height << std::endl;
        wl_egl_window_resize(m_context_data->egl_window, width, height, 0, 0);  
    }

    static void check_egl(const char* msg)
    {
        EGLint err = eglGetError();
        if (err != EGL_SUCCESS)
        {
            std::cerr << msg << " : " << err << std::endl;
        }
    }

    void OpenGLWaylandContext::swap_buffers(){
        if (m_context_data == nullptr || m_context_data->egl_window == nullptr) return;
        make_current();
        eglSwapBuffers(s_globals->egl_display, m_context_data->egl_surface);
        check_egl("Failed to swap EGL buffers");
    }

    void OpenGLWaylandContext::destroy(){
        cleanup();
        if(s_instance_count == 0){
            cleanup_wayland_opengl_globals(s_globals);
            delete s_globals;
            s_globals = nullptr;
        }
    }

    void OpenGLWaylandContext::cleanup(){
        if (m_context_data)
        {
            if (m_context_data->egl_surface != EGL_NO_SURFACE) {
                eglDestroySurface(s_globals->egl_display, m_context_data->egl_surface);
                m_context_data->egl_surface = EGL_NO_SURFACE;
            }

            if (m_context_data->egl_context != EGL_NO_CONTEXT) {
                eglDestroyContext(s_globals->egl_display, m_context_data->egl_context);
                m_context_data->egl_context = EGL_NO_CONTEXT;
            }

            if (m_context_data->egl_window) {
                wl_egl_window_destroy(m_context_data->egl_window);
                m_context_data->egl_window = nullptr;
            }
            s_instance_count--;
            delete m_context_data;
            m_context_data = nullptr;
        }
        
    }
}