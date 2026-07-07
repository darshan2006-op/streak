#include "core/platform.h"

namespace streak{
    Platform Platform::s_instance;

    Platform& Platform::get_instance()
    {
        return s_instance;
    }

    PlatformValue Platform::get_platform_value() const
    {
#if defined(STREAK_PLATFORM_WINDOWS)
        return PlatformValue::Windows;
#elif defined(STREAK_PLATFORM_LINUX)
        return PlatformValue::Linux;
#else
        static_assert(false, "Unsupported platform");
#endif
    }
}
