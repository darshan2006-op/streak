#pragma once

#include <functional>

#include "events/event.h"
#include "events/event_queue.h"

namespace streak{
    namespace event{

        using EventHandler = std::function<bool(std::shared_ptr<Event>)>;

        class EventDispatcher{
            public:
                EventDispatcher(){
                    m_handler = nullptr;
                }

                void set_handler(EventHandler handler){
                    m_handler = handler;
                }

                void push(std::shared_ptr<Event> e){
                    m_event_queue.push(e);
                }

                void drain(){
                    if(m_handler == nullptr) return;
                    while (!m_event_queue.is_empty())
                    {
                        auto e = m_event_queue.try_pop();
                        if (!e) {
                            break;
                        }
                        if(m_handler(e)){
                            e->handled();
                        }
                    }                    
                }
            protected:
                EventHandler m_handler;
                EventQueue<std::shared_ptr<Event>> m_event_queue;
        };
    }
}