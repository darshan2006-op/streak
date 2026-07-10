#pragma once

#include <memory>

namespace streak{
    class Semaphore{
        public:
            Semaphore(const Semaphore&) = delete;
            Semaphore& operator=(const Semaphore&) = delete;
            virtual ~Semaphore() = default;

            static std::unique_ptr<Semaphore> create(int count);

            virtual void acquire() = 0;
            virtual void release() = 0;
        protected:
            Semaphore() = default;
    };
}