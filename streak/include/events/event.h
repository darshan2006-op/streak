#pragma once
#include <cinttypes>

namespace streak{
    namespace event{

        class Event{
            public:
                Event() = default;                
                
                virtual uint32_t get_event_type() = 0;

                bool is_handled(){
                    return m_handled;
                }

                void handled(){
                    m_handled = true;
                }

                virtual ~Event() = default;
            protected:
                bool m_handled;
        };
    }
}