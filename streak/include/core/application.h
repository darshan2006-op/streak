#include <memory>

namespace streak {
    class Application;
    using ApplicationPtr = std::unique_ptr<Application>;
    
    class Application{
        public:
            Application() = default;
            Application(const Application&) = delete;
            virtual ~Application() = default;

            virtual void on_init() = 0;
            virtual void on_update() = 0;
            virtual void on_exit() = 0;

            virtual bool should_exit() const = 0;
    };

    extern ApplicationPtr create_application();
}