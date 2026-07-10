#include "mt_sync/semaphore.h"

#include "core/platform.h"
#include "platform/linux/linux_semaphore.h"

#include <stdexcept>

namespace streak{
    std::unique_ptr<Semaphore> Semaphore::create(int count){
        switch (Platform::get_instance().get_platform_value())
        {
            case PlatformValue::Linux:
                return std::make_unique<LinuxSemaphore>(count);
            default:
                throw std::runtime_error("Unsupported platform for Semaphore");
        }
    }
}