#pragma once

#include <queue>
#include <mutex>
#include <atomic>
#include <memory>

#include "events/event.h"

namespace streak{
    namespace event{
        class EventQueue{
            public:

                EventQueue(): m_event_queue(), m_queue_mutex(){
                    change_empty(true);
                }

                bool is_empty() const {
                    return m_is_empty.load(std::memory_order_acquire);
                }

                void push(const std::shared_ptr<Event>& e) {
                    std::unique_lock push_lock(m_queue_mutex);

                    m_event_queue.push(e);

                    if(is_empty()){
                        change_empty(false);
                    }
                }

                std::shared_ptr<Event> pop(){
                    std::unique_lock pop_lock(m_queue_mutex);

                    auto event = std::move(m_event_queue.front());
                    m_event_queue.pop();

                    if(m_event_queue.empty()){
                        change_empty(true);
                    }

                    return event;
                }

            protected:
                inline void change_empty(bool value){
                    m_is_empty.store(value, std::memory_order_release);
                }

                std::queue<std::shared_ptr<Event>> m_event_queue;
                std::mutex m_queue_mutex;
                std::atomic_bool m_is_empty;
        };
    }
}