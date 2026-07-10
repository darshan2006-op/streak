#pragma once
#include "core/window.h"

#include <atomic>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <functional>
#include <condition_variable>

#include <sys/eventfd.h>

struct wl_display;
struct wl_registry;
struct wl_compositor;
struct xdg_wm_base;

struct wl_surface;
struct xdg_surface;
struct xdg_toplevel;

namespace streak{

    class WaylandWindow;
    class WaylandWindowSystem;
    using WaylandWindowPtr = std::unique_ptr<WaylandWindow>;
    using WaylandWindowSystemPtr = std::unique_ptr<WaylandWindowSystem>;

    struct WaylandWindowGlobals;
    struct WaylandWindowData;

    struct WaylandWindowData{
        wl_surface* surface = nullptr;
        xdg_surface* xdg_surface = nullptr;
        xdg_toplevel* toplevel = nullptr;

        bool configured;
        uint32_t width = 0, height = 0;
    };

    struct WaylandWindowGlobals{
        wl_display* display = nullptr;
        wl_registry* registry = nullptr;

        wl_compositor* compositor = nullptr;
        xdg_wm_base* xdg_wm_base = nullptr;
    };

    class WaylandWindowSystem: public WindowSystem{
        public:
            WaylandWindowSystem() ;
            WaylandWindowSystem(const WaylandWindowSystem&) = delete;
            WaylandWindowSystem& operator=(const WaylandWindowSystem&) = delete;
            virtual ~WaylandWindowSystem();

            Window* create_window(const WindowOptions& options) override;
            void destroy_window(Window* window) override;

            uint32_t get_window_count() override{
                std::unique_lock lock(m_data_mutex);
                return static_cast<uint32_t>(m_windows.size());
            }

            void push_task(std::function<void()> task);

            WaylandWindowGlobals* get_globals(){
                return m_globals;
            }

            void init() override;
            void destroy() override;

            static WaylandWindowSystem& get();
        
        private:
            void cleanup();
            void drain_tasks();

            std::vector<WaylandWindowPtr> m_windows;
            std::queue<std::function<void()>> m_task_queue;
            WaylandWindowGlobals* m_globals;
            int m_event_fd;

            std::mutex m_data_mutex;
            std::thread m_event_thread;
            std::atomic<bool> m_running;
    };

    class WaylandWindow: public Window{
        public:
            WaylandWindow(const WindowOptions& options);
            WaylandWindow(const WaylandWindow&) = delete;
            WaylandWindow& operator=(const WaylandWindow&) = delete;
            virtual ~WaylandWindow() = default;

            void init();

            WaylandWindowData* get_window_data();
            
            void* get_native_window_data() override{
                std::unique_lock lock(m_window_mutex);
                return static_cast<void*>(m_window);
            }

            WindowOptions* get_options(){
                return &m_options;
            }

            void wait_for_configure(){
                std::unique_lock lock(m_window_mutex);
                m_window_state_cv.wait(lock, [this](){
                    auto window = m_window;
                    return window && window->configured;
                });
            }

            void wait_for_close(){
                std::unique_lock lock(m_window_mutex);
                m_window_state_cv.wait(lock, [this](){
                    return m_closed;
                });
            }

            void notify_close() override{
                {
                    std::unique_lock lock(m_window_mutex);
                    m_closed = true;
                }
                notify_configured();
            }

            std::mutex& get_window_mutex(){
                return m_window_mutex;
            }

            void notify_configured(){
                m_window_state_cv.notify_all();
            }

            void destroy();
        private:
            void cleanup();

            WindowOptions m_options;
            std::mutex m_window_mutex;
            std::condition_variable m_window_state_cv;
            bool m_closed ;
            WaylandWindowData* m_window;
    };
}
