#include "scheduler.h"
#include "kheap.h"
#include "serial.h"

extern "C" void switch_context(uint64_t* old_rsp, uint64_t new_rsp);

namespace Forge {
    namespace Kernel {
        namespace {
            constexpr int MAX_TASKS = 8;
            constexpr uint64_t TASK_STACK_SIZE = 8192;
            constexpr uint64_t FRAME_SLOTS = 8; // r15,r14,r13,r12,rbx,rbp,entry,exit

            struct Task {
                uint64_t rsp;
                uint64_t id;
                bool alive;
            };

            Task tasks[MAX_TASKS];
            int task_count = 0;
            int current = 0;
        }

        void scheduler_init() {
            for (int i = 0; i < MAX_TASKS; i++) {
                tasks[i].rsp = 0;
                tasks[i].id = (uint64_t)i;
                tasks[i].alive = false;
            }
            // Task 0 = contexto de execução atual (kernel_main / idle).
            // O rsp dela será preenchido pelo primeiro switch_context.
            tasks[0].alive = true;
            task_count = 1;
            current = 0;
            Interrupts::klog("sched: initialized with idle task 0");
        }

        int task_create(void (*entry)()) {
            if (task_count >= MAX_TASKS) return -1;
            void* stack = Memory::kmalloc(TASK_STACK_SIZE);
            if (stack == nullptr) return -1;

            // Topo alinhado a 16 bytes; o frame sintético fica ABAIXO do
            // topo (stacks crescem para baixo!). Escrever acima do topo
            // corromperia o header do próximo bloco do heap.
            uint64_t top = ((uint64_t)stack + TASK_STACK_SIZE) & ~15ULL;
            uint64_t* sp = (uint64_t*)(top - FRAME_SLOTS * sizeof(uint64_t));

            // Layout esperado pelo switch_context ao "retomar" pela 1ª vez:
            // [r15][r14][r13][r12][rbx][rbp][entry][task_exit]
            sp[0] = 0;                      // r15
            sp[1] = 0;                      // r14
            sp[2] = 0;                      // r13
            sp[3] = 0;                      // r12
            sp[4] = 0;                      // rbx
            sp[5] = 0;                      // rbp
            sp[6] = (uint64_t)entry;        // "retorno" do primeiro switch
            sp[7] = (uint64_t)task_exit;    // caso entry() retorne

            int slot = task_count++;
            tasks[slot].rsp = (uint64_t)sp;
            tasks[slot].alive = true;
            Interrupts::klog_hex("sched: created task", (uint64_t)slot);
            Interrupts::klog_hex("sched: task rsp", (uint64_t)sp);
            return slot;
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
