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
#include "user_program.h"

extern "C" uint64_t multiboot2_info_addr;

extern "C" void kernel_main() {
    Forge::Console::init();
    Forge::Interrupts::serial_init();
    Forge::Interrupts::klog("kernel_main entered");

    Forge::Console::set_color(Forge::Console::LightGreen, Forge::Console::Black);
    Forge::Console::println("ForgeOS v0.6 - Phase 6: User Mode + Syscalls");
    Forge::Console::set_color(Forge::Console::White, Forge::Console::Black);

    Forge::Console::println("Initializing GDT and TSS...");
    Forge::CPU::gdt_init();
    Forge::Console::println("GDT loaded successfully (user segments added).");

    Forge::Console::println("Initializing PIC...");
    Forge::Interrupts::pic_init();
    Forge::Console::println("PIC remapped and fully masked.");

    Forge::Console::println("Initializing IDT...");
    Forge::Interrupts::idt_init();
    Forge::Console::println("IDT loaded successfully (syscall gate 0x80 DPL=3).");

    Forge::Console::println("Enabling hardware interrupts...");
    __asm__ __volatile__("sti");

    Forge::Console::println("Initializing Physical Memory Manager...");
    Forge::Memory::pmm_init(multiboot2_info_addr);
    Forge::Console::print("  Total usable RAM: ");
    Forge::Console::print_dec(Forge::Memory::pmm_total_pages() * 4);
    Forge::Console::println(" KiB");

    Forge::Console::println("Initializing Virtual Memory Manager...");
    Forge::Memory::vmm_init();
    Forge::Console::println("VMM online (CR3 switched, user bit on low map).");

    Forge::Console::println("Initializing kernel heap (kmalloc)...");
    Forge::Memory::kheap_init();
    Forge::Console::println("Kernel heap online.");

    Forge::Console::println("Initializing Scheduler + Timer (100 Hz)...");
    Forge::Kernel::scheduler_init();
    Forge::Drivers::timer_init(100);

    Forge::Console::println("Launching user tasks in ring 3...");
    Forge::Kernel::task_create_user(Forge::User::user_main_a);
    Forge::Kernel::task_create_user(Forge::User::user_main_b);
    Forge::Console::println("Scheduler live. Idle task looping (timer preempts).");
    Forge::Console::println("");

    // Task 0 (idle): espera interrupções. O timer preempta para as tasks user.
    while (true) {
        __asm__ __volatile__("hlt");
    }
}
