#include "idt.h"
#include "io_ports.h"
#include "io.h"
#include "syscall.h"

extern "C" void isr0(); extern "C" void isr1(); extern "C" void isr2(); extern "C" void isr3();
extern "C" void isr4(); extern "C" void isr5(); extern "C" void isr6(); extern "C" void isr7();
extern "C" void isr8(); extern "C" void isr9(); extern "C" void isr10(); extern "C" void isr11();
extern "C" void isr12(); extern "C" void isr13(); extern "C" void isr14(); extern "C" void isr15();
extern "C" void isr16(); extern "C" void isr17(); extern "C" void isr18(); extern "C" void isr19();
extern "C" void isr20(); extern "C" void isr21(); extern "C" void isr22(); extern "C" void isr23();
extern "C" void isr24(); extern "C" void isr25(); extern "C" void isr26(); extern "C" void isr27();
extern "C" void isr28(); extern "C" void isr29(); extern "C" void isr30(); extern "C" void isr31();
extern "C" void isr128();
extern "C" void irq0(); extern "C" void irq1(); extern "C" void irq2(); extern "C" void irq3();
extern "C" void irq4(); extern "C" void irq5(); extern "C" void irq6(); extern "C" void irq7();
extern "C" void irq8(); extern "C" void irq9(); extern "C" void irq10(); extern "C" void irq11();
extern "C" void irq12(); extern "C" void irq13(); extern "C" void irq14(); extern "C" void irq15();

namespace Forge {
    namespace Interrupts {
        struct IDTEntry {
            uint16_t base_low;
            uint16_t sel;
            uint8_t ist;
            uint8_t flags;
            uint16_t base_mid;
            uint32_t base_high;
            uint32_t reserved;
        } __attribute__((packed));

        IDTEntry idt[256];

        struct {
            uint16_t limit;
            uint64_t base;
        } __attribute__((packed)) idt_ptr;

        void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
            idt[num].base_low = base & 0xFFFF;
            idt[num].base_mid = (base >> 16) & 0xFFFF;
            idt[num].base_high = (base >> 32) & 0xFFFFFFFF;
            idt[num].sel = sel;
            idt[num].ist = 0;
            idt[num].flags = flags;
            idt[num].reserved = 0;
        }

        void idt_init() {
            idt_ptr.limit = sizeof(idt) - 1;
            idt_ptr.base = (uint64_t)&idt;

            uint64_t isr_handlers[32] = {
                (uint64_t)isr0,  (uint64_t)isr1,  (uint64_t)isr2,  (uint64_t)isr3,
                (uint64_t)isr4,  (uint64_t)isr5,  (uint64_t)isr6,  (uint64_t)isr7,
                (uint64_t)isr8,  (uint64_t)isr9,  (uint64_t)isr10, (uint64_t)isr11,
                (uint64_t)isr12, (uint64_t)isr13, (uint64_t)isr14, (uint64_t)isr15,
                (uint64_t)isr16, (uint64_t)isr17, (uint64_t)isr18, (uint64_t)isr19,
                (uint64_t)isr20, (uint64_t)isr21, (uint64_t)isr22, (uint64_t)isr23,
                (uint64_t)isr24, (uint64_t)isr25, (uint64_t)isr26, (uint64_t)isr27,
                (uint64_t)isr28, (uint64_t)isr29, (uint64_t)isr30, (uint64_t)isr31
            };
            uint64_t irq_handlers[16] = {
                (uint64_t)irq0,  (uint64_t)irq1,  (uint64_t)irq2,  (uint64_t)irq3,
                (uint64_t)irq4,  (uint64_t)irq5,  (uint64_t)irq6,  (uint64_t)irq7,
                (uint64_t)irq8,  (uint64_t)irq9,  (uint64_t)irq10, (uint64_t)irq11,
                (uint64_t)irq12, (uint64_t)irq13, (uint64_t)irq14, (uint64_t)irq15
            };

            for (int i = 0; i < 32; i++) {
                idt_set_gate((uint8_t)i, isr_handlers[i], 0x08, 0x8E);
            }
            for (int i = 0; i < 16; i++) {
                idt_set_gate((uint8_t)(32 + i), irq_handlers[i], 0x08, 0x8E);
            }

            // Syscall gate: DPL=3 (0x60) para que ring 3 possa usar int 0x80
            idt_set_gate(0x80, (uint64_t)isr128, 0x08, 0xEE);

            __asm__ __volatile__("lidt %0" : : "m"(idt_ptr));
        }

        const char* exception_messages[] = {
            "Division By Zero", "Debug", "Non Maskable Interrupt", "Breakpoint",
            "Into Detected Overflow", "Out of Bounds", "Invalid Opcode", "No Coprocessor",
            "Double Fault", "Coprocessor Segment Overrun", "Bad TSS", "Segment Not Present",
            "Stack Fault", "General Protection Fault", "Page Fault", "Unknown Interrupt",
            "Coprocessor Fault", "Alignment Check", "Machine Check", "Reserved",
            "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
            "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"
        };

        extern "C" void fault_handler(InterruptFrame* frame) {
            // int 0x80: syscall vinda de qualquer ring
            if (frame->int_no == 0x80) {
                Syscall::syscall_handler(frame);
                return;
            }

            if (frame->int_no < 32) {
                Console::set_color(Console::White, Console::Red);
                Console::print("\nKERNEL PANIC: ");
                Console::print(exception_messages[frame->int_no]);
                Console::print(" (Int ");
                Console::print_dec(frame->int_no);
                Console::print(") at RIP ");
                Console::print_hex(frame->rip);
                Console::println("");

                Console::set_color(Console::LightGrey, Console::Black);
                Console::print("Error Code: ");
                Console::print_hex(frame->err_code);
                Console::println("\nSystem Halted.");

                while (true) { __asm__ __volatile__("cli; hlt"); }
            }
        }
    }
}
