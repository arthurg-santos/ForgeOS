#include "io.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "serial.h"

extern "C" void kernel_main() {
    Forge::Console::init();
    Forge::Interrupts::serial_init();
    Forge::Interrupts::klog("kernel_main entered");

    Forge::Console::set_color(Forge::Console::LightGreen, Forge::Console::Black);
    Forge::Console::println("ForgeOS v0.2 - Phase 2: CPU & Interrupts");
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
    Forge::Interrupts::klog("executing sti");
    __asm__ __volatile__("sti");

    Forge::Console::println("\nSystem initialized. Triggering Breakpoint Exception (Int 3)...");
    __asm__ __volatile__("int $3");

    while (true) {
        __asm__ __volatile__("hlt");
    }
}
