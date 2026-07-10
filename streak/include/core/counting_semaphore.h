#pragma once

#include <mutex>
#include <condition_variable>

namespace streak {

class CountingSemaphore {
public:
    explicit CountingSemaphore(int initial_count) : m_count(initial_count) {}

    void acquire() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this]() { return m_count > 0; });
        --m_count;
    }

    // Non-blocking variant, useful if you ever want to poll instead of block.
    bool try_acquire() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_count > 0) {
            --m_count;
            return true;
        }
        return false;
    }

    void release(int n = 1) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_count += n;
        }
        // notify outside the lock to avoid waking a thread that
        // immediately blocks again on the same mutex
        if (n == 1) {
            m_cv.notify_one();
        } else {
            m_cv.notify_all();
        }
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    int m_count;
};

}