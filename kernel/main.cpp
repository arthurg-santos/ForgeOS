#include "io.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "serial.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"

extern "C" uint64_t multiboot2_info_addr;

extern "C" void kernel_main() {
    Forge::Console::init();
    Forge::Interrupts::serial_init();
    Forge::Interrupts::klog("kernel_main entered");

    Forge::Console::set_color(Forge::Console::LightGreen, Forge::Console::Black);
    Forge::Console::println("ForgeOS v0.4 - Phase 4: Virtual Memory + Kernel Heap");
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

    Forge::Console::println("Initializing Virtual Memory Manager...");
    Forge::Memory::vmm_init();
    Forge::Console::print("  PML4 (phys):      ");
    Forge::Console::print_hex(Forge::Memory::vmm_current_pml4());
    Forge::Console::println("");

    Forge::Console::println("Initializing kernel heap (kmalloc)...");
    Forge::Memory::kheap_init();

    void* a = Forge::Memory::kmalloc(64);
    void* b = Forge::Memory::kmalloc(4096);
    void* c = Forge::Memory::kmalloc(128);

    Forge::Console::print("  kmalloc(64)   -> ");
    Forge::Console::print_hex((uint64_t)a);
    Forge::Console::println("");
    Forge::Console::print("  kmalloc(4096) -> ");
    Forge::Console::print_hex((uint64_t)b);
    Forge::Console::println("");
    Forge::Console::print("  kmalloc(128)  -> ");
    Forge::Console::print_hex((uint64_t)c);
    Forge::Console::println("");

    // Write/read test no bloco de 4 KiB (higher-half!)
    volatile uint8_t* pb = (volatile uint8_t*)b;
    bool heap_ok = true;
    for (uint64_t i = 0; i < 4096; i++) pb[i] = (uint8_t)(i & 0xFF);
    for (uint64_t i = 0; i < 4096; i++) {
        if (pb[i] != (uint8_t)(i & 0xFF)) { heap_ok = false; break; }
    }
    Forge::Console::print("  Heap write/read test: ");
    Forge::Console::println(heap_ok ? "OK" : "FAILED");

    Forge::Memory::kfree(b);
    void* d = Forge::Memory::kmalloc(2048);
    Forge::Console::print("  kmalloc(2048) after free -> ");
    Forge::Console::print_hex((uint64_t)d);
    Forge::Console::println("  (reuses freed block)");

    Forge::Console::print("  Heap: ");
    Forge::Console::print_dec(Forge::Memory::kheap_used());
    Forge::Console::print(" bytes used of ");
    Forge::Console::print_dec(Forge::Memory::kheap_capacity());
    Forge::Console::println(" bytes mapped");

    Forge::Console::println("\nSystem initialized. Triggering Breakpoint Exception (Int 3)...");
    __asm__ __volatile__("int $3");

    while (true) {
        __asm__ __volatile__("hlt");
    }
}
