#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"

namespace Forge {
    namespace Kernel {
        void scheduler_init();
        int task_create(void (*entry)());
        int process_create(const uint8_t* elf_blob, uint64_t size);
        void schedule();
        void yield();
        void task_exit();
        uint64_t current_task_id();
    }
}

#endif
