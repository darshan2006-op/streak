#pragma once

#include <memory>
#include <cstdint>
#include <string>

#include "events/event_dispatcher.h"

namespace streak{
    class Window;
    class WindowSystem;

    struct WindowOptions{
        uint32_t width;
        uint32_t height;
        std::string title;

        event::EventDispatcher* event_dispatcher;
    };

    class WindowSystem{
        public:
            virtual ~WindowSystem() = default;

            virtual Window* create_window(const WindowOptions& options) = 0;
            virtual void destroy_window(Window* window) = 0;

            virtual void init() = 0;
            virtual void destroy() = 0;

            virtual uint32_t get_window_count() = 0;
            
            
            static WindowSystem& get();
            
            protected:
            WindowSystem() = default;
            WindowSystem(const WindowSystem&) = delete;
        };
        
        class Window{
            public:
            Window(const Window&) = delete;
            Window& operator=(const Window&) = delete;
            virtual ~Window() = default;
            
            virtual void* get_native_window_data() = 0;
            
        protected:
            Window() = default;
    };

}
