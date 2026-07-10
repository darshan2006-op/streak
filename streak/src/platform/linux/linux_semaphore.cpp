#include "platform/linux/linux_semaphore.h"

#include <unistd.h>
#include <errno.h>


namespace streak{
    LinuxSemaphore::LinuxSemaphore(int count){
        int rc = sem_init(&sem, 0, static_cast<unsigned>(count));
        (void)rc;
    }

    void LinuxSemaphore::acquire(){
        int rc;
        do {
            rc = sem_wait(&sem);
        } while (rc == -1 && errno == EINTR);
    }

    void LinuxSemaphore::release(){
        int rc = sem_post(&sem);
        (void)rc;
    }

    LinuxSemaphore::~LinuxSemaphore(){
        sem_destroy(&sem);
    }
}