#include "core/window.h"
#include "platform/linux/wayland_window.h"
#include "core/platform.h"
#include <stdexcept>

namespace streak{

    WindowSystem& WindowSystem::get(){
        switch (Platform::get_instance().get_platform_value())
        {
            case PlatformValue::Linux:
                return WaylandWindowSystem::get();

        }
    }
}