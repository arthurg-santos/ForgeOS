#include "pic.h"
#include "io_ports.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

namespace Forge {
    namespace Interrupts {
        void pic_init() {
            outb(PIC1_COMMAND, 0x11); // ICW1: Init + ICW4
            outb(PIC2_COMMAND, 0x11);
            io_wait();

            outb(PIC1_DATA, 0x20); // ICW2: Master offset 0x20 (32)
            outb(PIC2_DATA, 0x28); // ICW2: Slave offset 0x28 (40)
            io_wait();

            outb(PIC1_DATA, 0x04); // ICW3: Slave at IRQ2
            outb(PIC2_DATA, 0x02); // ICW3: Cascade identity
            io_wait();

            outb(PIC1_DATA, 0x01); // ICW4: 8086 mode
            outb(PIC2_DATA, 0x01);
            io_wait();

            // MÁSCARA TOTAL: nenhuma IRQ passa até existir handler para ela.
            outb(PIC1_DATA, 0xFF);
            outb(PIC2_DATA, 0xFF);
        }

        void pic_send_eoi(uint8_t irq) {
            if (irq >= 8) outb(PIC2_COMMAND, 0x20);
            outb(PIC1_COMMAND, 0x20);
        }

        void pic_unmask(uint8_t irq) {
            uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
            uint8_t mask = inb(port);
            mask &= (uint8_t)~(1 << (irq % 8));
            outb(port, mask);
        }

        void pic_mask(uint8_t irq) {
            uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
            uint8_t mask = inb(port);
            mask |= (uint8_t)(1 << (irq % 8));
            outb(port, mask);
        }
    }
}
