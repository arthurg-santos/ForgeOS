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

// Blob ELF embutido na imagem do kernel (ld -r -b binary)
extern "C" uint8_t _binary_init_elf_start[];
extern "C" uint8_t _binary_init_elf_end[];

extern "C" void kernel_main() {
    Forge::Console::init();
    Forge::Interrupts::serial_init();
    Forge::Interrupts::klog("kernel_main entered");

    Forge::Console::set_color(Forge::Console::LightGreen, Forge::Console::Black);
    Forge::Console::println("ForgeOS v0.7 - Phase 7: ELF Loader + Address Spaces");
    Forge::Console::set_color(Forge::Console::White, Forge::Console::Black);

    Forge::Console::println("Initializing GDT and TSS...");
    Forge::CPU::gdt_init();
    Forge::Console::println("GDT loaded successfully.");

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
    Forge::Console::println("VMM online (kernel-only low map).");

    Forge::Console::println("Initializing kernel heap (kmalloc)...");
    Forge::Memory::kheap_init();
    Forge::Console::println("Kernel heap online.");

    Forge::Console::println("Initializing Scheduler + Timer (100 Hz)...");
    Forge::Kernel::scheduler_init();
    Forge::Drivers::timer_init(100);

    uint64_t elf_size = (uint64_t)(_binary_init_elf_end - _binary_init_elf_start);
    Forge::Console::print("Loading ELF process image (");
    Forge::Console::print_dec(elf_size);
    Forge::Console::println(" bytes)...");

    Forge::Kernel::process_create(_binary_init_elf_start, elf_size);
    Forge::Kernel::process_create(_binary_init_elf_start, elf_size);

    Forge::Console::println("Scheduler live. Two isolated processes from one ELF.");
    Forge::Console::println("");

    while (true) {
        __asm__ __volatile__("hlt");
    }
}
