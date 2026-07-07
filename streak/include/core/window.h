#pragma once

#include <memory>
#include <cstdint>
#include <string>

namespace streak{
    class Window;
    class WindowSystem;

    struct WindowOptions{
        uint32_t width;
        uint32_t height;
        std::string title;
    };

    class WindowSystem{
        public:
            virtual ~WindowSystem() = default;

            virtual Window* create_window(const WindowOptions& options) = 0;
            virtual void destroy_window(Window* window) = 0;

            virtual void init() = 0;
            virtual void destroy() = 0;

            virtual uint32_t get_window_count() const = 0;

            static WindowSystem& get();

        protected:
            WindowSystem() = default;
            WindowSystem(const WindowSystem&) = delete;
    };

    class Window{
        public:
            virtual ~Window() = default;

            virtual bool should_close() const = 0;
        protected:
            Window() = default;
            Window(const Window&) = delete;
    };

}
