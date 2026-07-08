#pragma once

#include <cstdint>

namespace streak
{
    enum class PlatformValue: std::uint8_t
    {
        Windows,
        Linux
    };

    class Platform{
        public:
            static Platform& get_instance();
            PlatformValue get_platform_value() const;
        private:
            Platform() = default;
            Platform(const Platform&) = delete;

            static Platform s_instance;
    };
}