#pragma once

#include <semaphore.h>
#include <pthread.h>
#include "mt_sync/semaphore.h"

namespace streak{
    class LinuxSemaphore : public Semaphore{
        public:
            LinuxSemaphore(int count);
            LinuxSemaphore(const LinuxSemaphore&) = delete;
            LinuxSemaphore& operator=(const LinuxSemaphore&) = delete;
            virtual ~LinuxSemaphore();

            void acquire() override;
            void release() override;
        protected:
            sem_t sem;
    };
}