#include "platform/linux/wayland_window.h"

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

#include <iostream>
#include <condition_variable>
#include <cstring>

#include <sys/poll.h>
#include <unistd.h>

#include "events/window_event.h"

namespace streak{

    static wl_registry_listener registry_listener;

    enum class WaylandWindowError{
        None,
        SurfaceCreationFailed,
        XdgSurfaceCreationFailed,
        ToplevelSurfaceCreationFailed,
    };

    enum class WaylandGlobalsError{
        None,
        DisplayConnectFailed,
        RegistryRetrivalFailed,
        CompositorMissing,
        XdgWmBaseMissing,
    };

    template<typename T>
    struct WaylandError{
        T error;
        std::string message;
    };

    struct WaylandWindowData{
        wl_surface* surface = nullptr;
        xdg_surface* xdg_surface = nullptr;
        xdg_toplevel* toplevel = nullptr;

        uint32_t width = 0, height = 0;
    };

    struct WaylandWindowGlobals{
        wl_display* display = nullptr;
        wl_registry* registry = nullptr;

        wl_compositor* compositor = nullptr;
        xdg_wm_base* xdg_wm_base = nullptr;
    };

    WaylandWindowSystem::WaylandWindowSystem():m_globals(nullptr){
        m_running.store(false, std::memory_order_relaxed);
    }

    WaylandWindowSystem& WaylandWindowSystem::get(){
        static WaylandWindowSystem instance;
        return instance;
    }

    static void xdg_wm_base_ping(void*, xdg_wm_base* base, uint32_t serial) {
        xdg_wm_base_pong(base, serial);
    }
    static constexpr xdg_wm_base_listener wm_base_listener = { .ping = xdg_wm_base_ping };

    void WaylandWindowSystem::push_task(std::function<void()> task){
        {
            std::lock_guard<std::mutex> task_lock(m_task_mutex);
            m_task_queue.push(task);
        }
        
        uint64_t value = 1;
        eventfd_write(m_event_fd, value);
    }

    static void registry_global(void *data, wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version){
        auto globals = static_cast<WaylandWindowGlobals*>(data);
        
        if(strcmp(interface, wl_compositor_interface.name) == 0) {
            globals->compositor = static_cast<wl_compositor*>(wl_registry_bind(wl_registry, name, &wl_compositor_interface, 4));
        }

        if (strcmp(interface, xdg_wm_base_interface.name) == 0)
        {
            globals->xdg_wm_base = static_cast<xdg_wm_base*>(wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 1));
        }
    }

    static void registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name){

    }

    static WaylandError<WaylandGlobalsError> initialize_wayland_globals(WaylandWindowGlobals* globals) {
        WaylandError<WaylandGlobalsError> result;

        result.error = WaylandGlobalsError::None;
        result.message = "successfully initialized display and registry";

        globals->display = wl_display_connect(nullptr);

        if(!globals->display){
            result.error = WaylandGlobalsError::DisplayConnectFailed;
            result.message = "unable to connect display";
            return result;
        }
        
        globals->registry = wl_display_get_registry(globals->display);

        if(!globals->registry){
            result.error = WaylandGlobalsError::RegistryRetrivalFailed;
            result.message = "unable to retrive registry";
            return result;
        }

        registry_listener.global = registry_global;
        registry_listener.global_remove = registry_global_remove;

        wl_registry_add_listener(globals->registry, &registry_listener, globals);

        return result;
    }

    void WaylandWindowSystem::init() {

        m_globals = new WaylandWindowGlobals();

        this->m_event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

        bool is_ready = false;
        std::mutex ready_mutex;
        std::condition_variable ready_cv;

        m_event_thread = std::thread([this, &is_ready, &ready_mutex, &ready_cv](){
            auto globals = this->m_globals;

            auto result = initialize_wayland_globals(globals);

            std::cout << result.message << std::endl;

            if(result.error != WaylandGlobalsError::None){
                cleanup();
                return;
            }
            wl_display_roundtrip(globals->display);

            if (!globals->compositor || !globals->xdg_wm_base)
            {
                cleanup();
                return;
            }
            
            xdg_wm_base_add_listener(globals->xdg_wm_base, &wm_base_listener, nullptr);

            int wayland_fd = wl_display_get_fd(globals->display);
            
            pollfd fds[2];
            
            fds[0].fd = wayland_fd;
            fds[0].events = POLLIN;
            
            fds[1].fd = this->m_event_fd;
            fds[1].events = POLLIN;
            
            m_running.store(true, std::memory_order_relaxed);

            bool has_notified_outer_thread = false;

            while (m_running.load(std::memory_order_relaxed))
            {

                if(!has_notified_outer_thread){
                    {
                        std::unique_lock ready_lock(ready_mutex);
                        is_ready = true;
                    }
                    ready_cv.notify_all();
                    has_notified_outer_thread = true;
                    std::cout << "ready" << std::endl;
                }

                wl_display_dispatch_pending(globals->display);

                if(wl_display_flush(globals->display) < 0){
                    break;
                }

                while (wl_display_prepare_read(globals->display) != 0)
                {
                    wl_display_dispatch_pending(globals->display);
                }

                int ret = poll(fds, 2, -1);
                if (ret < 0)
                {
                    wl_display_cancel_read(globals->display);

                    if (errno == EINTR)
                        continue;

                    break;
                }

                if (fds[0].revents & POLLIN)
                {
                    wl_display_read_events(globals->display);
                }

                if (fds[1].revents & POLLIN)
                {
                    wl_display_cancel_read(globals->display);

                    uint64_t value;
                    eventfd_read(this->m_event_fd, &value);
                    drain_tasks();

                    continue;
                }
                
                wl_display_dispatch_pending(globals->display);
            }
            
            cleanup();

        });

        {
            std::unique_lock ready_lock(ready_mutex);
            ready_cv.wait(ready_lock, [&is_ready](){
                return is_ready;
            });
        }
    }

    void WaylandWindowSystem::destroy() {
        if(!m_globals) return;
        std::cout << "destroying" << std::endl;

        m_running.store(false, std::memory_order_relaxed);

        eventfd_write(m_event_fd, 1);

        if(m_event_thread.joinable()) m_event_thread.join();

        close(m_event_fd);
        m_event_fd = -1;
        
    }

    void WaylandWindowSystem::drain_tasks(){
        for(;;){
            std::function<void()> task;
            {
                std::lock_guard<std::mutex> task_lock(m_task_mutex);
                if(m_task_queue.empty()) break;
                task = std::move(m_task_queue.front());
                m_task_queue.pop();
            }
            try{
                task();
            }catch(std::exception e){
                std::cout << e.what() << std::endl;
            }
        }
        
    }

    void WaylandWindowSystem::cleanup() {
        std::cout << "cleaning up" << std::endl;
        if(m_globals){

            if(m_globals->xdg_wm_base){
                xdg_wm_base_destroy(m_globals->xdg_wm_base);
                m_globals->xdg_wm_base = nullptr;
            }
            
            if(m_globals->compositor){
                wl_compositor_destroy(m_globals->compositor);
                m_globals->compositor = nullptr;
            }

            if(m_globals->registry){
                wl_registry_destroy(m_globals->registry);
                m_globals->registry = nullptr;
            }

            if(m_globals->display){
                wl_display_disconnect(m_globals->display);
                m_globals->display = nullptr;
            }

            delete m_globals;
            m_globals = nullptr;
        }
    }

    Window* WaylandWindowSystem::create_window(const WindowOptions& options){
        auto window = std::make_unique<WaylandWindow>(options);

        auto raw = window.get();

        m_windows.emplace_back(std::move(window));

        std::mutex created_mutex;
        std::condition_variable created_cv;
        bool created_flag = false;

        push_task([raw, &created_mutex, &created_cv, &created_flag](){
            raw->init();
            {
                std::unique_lock created_lock(created_mutex);
                created_flag = true;
            }
            created_cv.notify_all();
            std::cout << "created window" << std::endl;
        });

        {
            std::unique_lock created_lock(created_mutex);
            created_cv.wait(created_lock, [&created_flag](){
                return created_flag;
            });
        }

        return raw;
    }

    WaylandWindowData* WaylandWindow::get_window_data(){
        auto window = m_window.load(std::memory_order_acquire);
        return window;
    }

    void WaylandWindowSystem::destroy_window(Window* window){
        auto wayland_window = static_cast<WaylandWindow*>(window);

        push_task([this, wayland_window](){
            wayland_window->destroy();
            m_windows.erase(std::remove_if(m_windows.begin(), m_windows.end(),
                [&](auto& w){ return w.get() == wayland_window; }), m_windows.end());
        });
    }

    WaylandWindowSystem::~WaylandWindowSystem() {
        destroy();
    }

    WaylandWindow::WaylandWindow(const WindowOptions& options): m_options(options), m_window(nullptr), m_should_close(false) {

    }

    WaylandError<WaylandWindowError> initialize_window(WaylandWindowData* data){
        auto globals = WaylandWindowSystem::get().get_globals();
        data->surface = wl_compositor_create_surface(globals->compositor);

        WaylandError<WaylandWindowError> result;
        result.error = WaylandWindowError::None;
        result.message = "successfully created window";

        if(!data->surface){
            result.error = WaylandWindowError::SurfaceCreationFailed;
            result.message = "unable to create surface";
            return result;
        }

        data->xdg_surface = xdg_wm_base_get_xdg_surface(globals->xdg_wm_base, data->surface);
        
        if(!data->xdg_surface){
            result.error = WaylandWindowError::XdgSurfaceCreationFailed;
            result.message = "unable to create xdg surface";
            return result;
        }

        data->toplevel = xdg_surface_get_toplevel(data->xdg_surface);
        if(!data->toplevel){
            result.error = WaylandWindowError::ToplevelSurfaceCreationFailed;
            result.message = "unable to create toplevel surface";
            return result;
        }

        return result;
    }

    static void xdg_surface_configure (void *data, xdg_surface *xdg_surface, uint32_t serial){
        xdg_surface_ack_configure(xdg_surface, serial);
    }

	static void toplevel_configure(void *data, xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, wl_array *states){
        auto window = static_cast<WaylandWindow*>(data);
        std::cout << "configure called" << std::endl;
        std::cout << width << " " << height << std::endl;

        auto window_data = window->get_window_data();
        if(width > 0 || height > 0){
            window_data->width = width;
            window_data->height = height;
        }else{
            window_data->width = width;
            window_data->height = height;
            window->get_options()->event_dispatcher->push(std::make_shared<event::WindowResizeEvent>(width, height));
        }

        window->get_options()->event_dispatcher->push(std::make_shared<event::WindowConfiguredEvent>());

        wl_surface_commit(window_data->surface);
    }

    static void toplevel_close(void *data, xdg_toplevel *xdg_toplevel){
        auto window = static_cast<WaylandWindow*>(data);
        window->get_options()->event_dispatcher->push(std::make_shared<event::WindowCloseEvent>());
        WaylandWindowSystem::get().destroy_window(window);
    }

    static void toplevel_configure_bounds(void *data, xdg_toplevel *xdg_toplevel, int32_t width, int32_t height){

    }

    static void toplevel_wm_capabilities(void *data, xdg_toplevel *xdg_toplevel, wl_array *capabilities){

    }

    static xdg_surface_listener xdg_surface_listener_object{
        .configure = xdg_surface_configure
    };
    static xdg_toplevel_listener xdg_toplevel_listener_object{
        .configure = toplevel_configure,
        .close = toplevel_close,
        .configure_bounds = toplevel_configure_bounds,
        .wm_capabilities = toplevel_wm_capabilities,
    };

    void WaylandWindow::init(){
        auto window = new WaylandWindowData();

        auto result = initialize_window(window);

        if(result.error != WaylandWindowError::None){
            std::cout << result.message << std::endl;
            cleanup();
            return;
        }

        xdg_surface_add_listener(window->xdg_surface, &xdg_surface_listener_object, window);
        xdg_toplevel_add_listener(window->toplevel, &xdg_toplevel_listener_object, this);

        wl_surface_commit(window->surface);

        m_window.store(window, std::memory_order_release);
    }

    bool WaylandWindow::should_close() const {
        return m_should_close.load(std::memory_order_relaxed);
    }

    void WaylandWindow::destroy(){
        cleanup();

        auto window = m_window.load(std::memory_order_acquire);
        delete window;
    }

    void WaylandWindow::cleanup(){
        auto window = m_window.load(std::memory_order_acquire);
        if (window)
        {
            if(window->toplevel){
                xdg_toplevel_destroy(window->toplevel);
                window->toplevel = nullptr;
            }

            if (window->xdg_surface)      
            {
                xdg_surface_destroy(window->xdg_surface);
                window->xdg_surface = nullptr;
            }

            if(window->surface){
                wl_surface_destroy(window->surface);
                window->surface = nullptr;
            }

            m_should_close.store(true, std::memory_order_relaxed);
        }
        
    }
}