#include "io.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "serial.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"
#include "scheduler.h"
#include "timer.h"

extern "C" uint64_t multiboot2_info_addr;

namespace {
    // Seção crítica simples: uma linha inteira sem preempção no meio.
    void println_atomic(const char* msg, uint64_t num, bool show_num) {
        __asm__ __volatile__("cli");
        Forge::Console::print(msg);
        if (show_num) Forge::Console::print_dec(num);
        Forge::Console::println("");
        __asm__ __volatile__("sti");
    }

    void task_a() {
        for (uint64_t i = 1; i <= 5; i++) {
            println_atomic("  [Task A] iteration ", i, true);
            Forge::Kernel::yield();
        }
        println_atomic("  [Task A] finished", 0, false);
        Forge::Kernel::task_exit();
    }

    void task_b() {
        for (uint64_t i = 1; i <= 5; i++) {
            println_atomic("  [Task B] iteration ", i, true);
            Forge::Kernel::yield();
        }
        println_atomic("  [Task B] finished", 0, false);
        Forge::Kernel::task_exit();
    }
}

extern "C" void kernel_main() {
    Forge::Console::init();
    Forge::Interrupts::serial_init();
    Forge::Interrupts::klog("kernel_main entered");

    Forge::Console::set_color(Forge::Console::LightGreen, Forge::Console::Black);
    Forge::Console::println("ForgeOS v0.5 - Phase 5: Timer + Scheduler");
    Forge::Console::set_color(Forge::Console::White, Forge::Console::Black);

    Forge::Console::println("Initializing GDT and TSS...");
    Forge::CPU::gdt_init();
    Forge::Console::println("GDT loaded successfully.");

    Forge::Console::println("Initializing PIC...");
    Forge::Interrupts::pic_init();
    Forge::Console::println("PIC remapped and fully masked.");

    Forge::Console::println("Initializing IDT...");
    Forge::Interrupts::idt_init();
    Forge::Console::println("IDT loaded successfully.");

    Forge::Console::println("Enabling hardware interrupts...");
    __asm__ __volatile__("sti");

    Forge::Console::println("Initializing Physical Memory Manager...");
    Forge::Memory::pmm_init(multiboot2_info_addr);
    Forge::Console::print("  Total usable RAM: ");
    Forge::Console::print_dec(Forge::Memory::pmm_total_pages() * 4);
    Forge::Console::println(" KiB");

    Forge::Console::println("Initializing Virtual Memory Manager...");
    Forge::Memory::vmm_init();
    Forge::Console::println("VMM online (CR3 switched).");

    Forge::Console::println("Initializing kernel heap (kmalloc)...");
    Forge::Memory::kheap_init();
    Forge::Console::println("Kernel heap online.");

    Forge::Console::println("Initializing Scheduler + Timer (100 Hz)...");
    Forge::Kernel::scheduler_init();
    Forge::Drivers::timer_init(100);
    Forge::Kernel::task_create(task_a);
    Forge::Kernel::task_create(task_b);
    Forge::Console::println("Scheduler live. Idle task looping (timer preempts).");
    Forge::Console::println("");

    // Task 0 (idle): espera interrupções. O timer preempta para A e B.
    while (true) {
        __asm__ __volatile__("hlt");
    }
}
