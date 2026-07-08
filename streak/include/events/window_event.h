#pragma once
#include "events/event.h"

namespace streak{
    namespace event{
        class WindowConfiguredEvent: public Event{
            public:
                static constexpr uint32_t event_type = 1;

                WindowConfiguredEvent() = default;

                uint32_t get_event_type() override {
                    return event_type;
                }

                ~WindowConfiguredEvent() = default;
        };

        class WindowCloseEvent: public Event{
            public:
                static constexpr uint32_t event_type = 2;

                WindowCloseEvent() = default;

                uint32_t get_event_type() override {
                    return event_type;
                }

                ~WindowCloseEvent() = default;
        };

        class WindowResizeEvent: public Event{
            public:
                static constexpr uint32_t event_type = 3;

                WindowResizeEvent(uint32_t width, uint32_t height):m_width(width), m_height(height){
                }

                uint32_t get_width() const {
                    return m_width;
                }

                uint32_t get_height() const {
                    return m_height;
                }

                uint32_t get_event_type() override{
                    return event_type;
                }

                ~WindowResizeEvent() = default;
            protected:
                uint32_t m_width, m_height;
        };
    }
}