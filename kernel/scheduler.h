#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"

namespace Forge {
    namespace Kernel {
        struct ProcEntry {
            uint64_t id;
            uint64_t alive;
            uint64_t is_user;
            char name[16];
        };

        void scheduler_init();
        int task_create(void (*entry)());
        int process_create(const uint8_t* elf_blob, uint64_t size, const char* name);
        void schedule();
        void yield();
        void task_exit();
        uint64_t current_task_id();
        uint64_t proc_count();
        bool proc_snapshot(int idx, ProcEntry* out);
    }
}

#endif
