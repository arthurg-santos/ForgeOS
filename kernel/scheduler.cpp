#include "scheduler.h"
#include "kheap.h"
#include "pmm.h"
#include "vmm.h"
#include "gdt.h"
#include "elf.h"
#include "serial.h"

extern "C" void switch_context(uint64_t* old_rsp, uint64_t new_rsp);
extern "C" uint64_t stack_top; // kernel stack da task 0 (boot stack)

namespace Forge {
    namespace Kernel {
        namespace {
            constexpr int MAX_TASKS = 8;
            constexpr uint64_t TASK_STACK_SIZE = 8192;
            constexpr uint64_t FRAME_SLOTS = 8; // r15,r14,r13,r12,rbx,rbp,entry,exit

            constexpr uint64_t USER_STACK_VA = 0x7FFF0000ULL;
            constexpr uint64_t USER_STACK_PAGES = 4; // 16 KiB

            struct Task {
                uint64_t rsp;            // rsp salvo pelo switch_context
                uint64_t id;
                bool alive;
                bool is_user;
                uint64_t pml4_phys;      // address space do processo
                uint64_t kernel_top;     // topo da kernel stack (=> TSS.rsp0)
                uint64_t user_stack_top; // topo da stack de ring 3 (VA)
                uint64_t user_entry;     // RIP inicial em ring 3
            };

            Task tasks[MAX_TASKS];
            int task_count = 0;
            int current = 0;

            void user_trampoline();
        }

        void scheduler_init() {
            for (int i = 0; i < MAX_TASKS; i++) {
                tasks[i].rsp = 0;
                tasks[i].id = (uint64_t)i;
                tasks[i].alive = false;
                tasks[i].is_user = false;
                tasks[i].pml4_phys = 0;
                tasks[i].kernel_top = 0;
                tasks[i].user_stack_top = 0;
                tasks[i].user_entry = 0;
            }
            tasks[0].alive = true;
            tasks[0].kernel_top = (uint64_t)&stack_top;
            tasks[0].pml4_phys = Memory::vmm_current_pml4();
            task_count = 1;
            current = 0;
            CPU::set_tss_rsp0(tasks[0].kernel_top);
            Interrupts::klog("sched: initialized with idle task 0");
        }

        int task_create(void (*entry)()) {
            if (task_count >= MAX_TASKS) return -1;
            void* stack = Memory::kmalloc(TASK_STACK_SIZE);
            if (stack == nullptr) return -1;

            uint64_t top = ((uint64_t)stack + TASK_STACK_SIZE) & ~15ULL;
            uint64_t* sp = (uint64_t*)(top - FRAME_SLOTS * sizeof(uint64_t));
            sp[0] = 0; sp[1] = 0; sp[2] = 0; sp[3] = 0; sp[4] = 0; sp[5] = 0;
            sp[6] = (uint64_t)entry;
            sp[7] = (uint64_t)task_exit;

            int slot = task_count++;
            tasks[slot].rsp = (uint64_t)sp;
            tasks[slot].alive = true;
            tasks[slot].is_user = false;
            tasks[slot].pml4_phys = Memory::vmm_current_pml4();
            tasks[slot].kernel_top = top;
            Interrupts::klog_hex("sched: created kernel task", (uint64_t)slot);
            return slot;
        }

        int process_create(const uint8_t* elf_blob, uint64_t size) {
            if (task_count >= MAX_TASKS) return -1;

            // 1. Address space isolado (PDPT privado + ramos de kernel)
            uint64_t space = Memory::vmm_new_address_space();
            if (space == 0) return -1;

            // 2. Carrega o ELF no espaço do processo (VAs >= 1 GiB)
            uint64_t entry = Elf::elf_load(space, elf_blob, size);
            if (entry == 0) return -1;

            // 3. User stack privada em USER_STACK_VA
            for (uint64_t i = 0; i < USER_STACK_PAGES; i++) {
                uint64_t pa = Memory::pmm_alloc_page();
                if (pa == 0) return -1;
                if (!Memory::vmm_map_page_in(space, USER_STACK_VA + i * 4096, pa,
                    Memory::VMM_PRESENT | Memory::VMM_WRITABLE | Memory::VMM_USER)) {
                    return -1;
                    }
            }

            // 4. Kernel stack + frame sintético -> trampoline -> ring 3
            void* kstack = Memory::kmalloc(TASK_STACK_SIZE);
            if (kstack == nullptr) return -1;
            uint64_t top = ((uint64_t)kstack + TASK_STACK_SIZE) & ~15ULL;
            uint64_t* sp = (uint64_t*)(top - FRAME_SLOTS * sizeof(uint64_t));
            sp[0] = 0; sp[1] = 0; sp[2] = 0; sp[3] = 0; sp[4] = 0; sp[5] = 0;
            sp[6] = (uint64_t)user_trampoline;
            sp[7] = (uint64_t)task_exit;

            int slot = task_count++;
            tasks[slot].rsp = (uint64_t)sp;
            tasks[slot].alive = true;
            tasks[slot].is_user = true;
            tasks[slot].pml4_phys = space;
            tasks[slot].kernel_top = top;
            tasks[slot].user_stack_top = USER_STACK_VA + USER_STACK_PAGES * 4096;
            tasks[slot].user_entry = entry;
            Interrupts::klog_hex("sched: created PROCESS", (uint64_t)slot);
            return slot;
        }

        namespace {
            void user_trampoline() {
                Task* t = &tasks[current];
                uint64_t ustack = t->user_stack_top;
                uint64_t uentry = t->user_entry;
                CPU::set_tss_rsp0(t->kernel_top);
                __asm__ __volatile__(
                    "mov $0x33, %%ax\n"   // User Data | RPL3
                    "mov %%ax, %%ds\n"
                    "mov %%ax, %%es\n"
                    "pushq $0x33\n"       // SS  (user)
                "pushq %0\n"          // RSP (user, VA privado)
                "pushq $0x202\n"      // RFLAGS (IF=1)
                "pushq $0x2B\n"       // CS  (user)
                "pushq %1\n"          // RIP (entry do ELF)
                "iretq\n"
                :
                : "r"(ustack), "r"(uentry)
                : "rax", "memory");
                __builtin_unreachable();
            }
        }

        void schedule() {
            if (task_count < 2) return;
            int next = current;
            for (int i = 1; i <= task_count; i++) {
                int cand = (current + i) % task_count;
                if (tasks[cand].alive) { next = cand; break; }
            }
            if (next == current) return;
            int prev = current;
            current = next;
            Memory::vmm_load_cr3(tasks[next].pml4_phys);
            CPU::set_tss_rsp0(tasks[next].kernel_top);
            switch_context(&tasks[prev].rsp, tasks[next].rsp);
        }

        void yield() { schedule(); }

        void task_exit() {
            tasks[current].alive = false;
            Interrupts::klog_hex("sched: task exited", tasks[current].id);
            while (true) schedule(); // nunca retorna
        }

        uint64_t current_task_id() { return tasks[current].id; }
    }
}
