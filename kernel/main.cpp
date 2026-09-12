#include "io.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "serial.h"
#include "pmm.h"

extern "C" uint64_t multiboot2_info_addr;

extern "C" void kernel_main() {
    Forge::Console::init();
    Forge::Interrupts::serial_init();
    Forge::Interrupts::klog("kernel_main entered");

    Forge::Console::set_color(Forge::Console::LightGreen, Forge::Console::Black);
    Forge::Console::println("ForgeOS v0.3 - Phase 3: Physical Memory");
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
    Forge::Console::print("  Free RAM:         ");
    Forge::Console::print_dec(Forge::Memory::pmm_free_pages() * 4);
    Forge::Console::println(" KiB");

    uint64_t page = Forge::Memory::pmm_alloc_page();
    Forge::Console::print("  Allocated page:   ");
    Forge::Console::print_hex(page);
    Forge::Console::println("");

    volatile uint64_t* probe = (volatile uint64_t*)page;
    probe[0] = 0xDEADBEEFCAFEBABEULL;
    bool probe_ok = (probe[0] == 0xDEADBEEFCAFEBABEULL);
    Forge::Console::print("  Write/read test:  ");
    Forge::Console::println(probe_ok ? "OK" : "FAILED");

    Forge::Memory::pmm_free_page(page);
    Forge::Console::print("  Free after release: ");
    Forge::Console::print_dec(Forge::Memory::pmm_free_pages() * 4);
    Forge::Console::println(" KiB");

    Forge::Console::println("\nSystem initialized. Triggering Breakpoint Exception (Int 3)...");
    __asm__ __volatile__("int $3");

    while (true) {
        __asm__ __volatile__("hlt");
    }
}
