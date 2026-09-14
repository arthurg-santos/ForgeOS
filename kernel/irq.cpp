#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"

namespace Forge {
    namespace Interrupts {
        extern "C" void irq_handler(InterruptFrame* frame) {
            uint8_t irq = (uint8_t)(frame->int_no - 32);
            if (irq == 0) {
                pic_send_eoi(0); // EOI antes de qualquer troca de contexto
                Drivers::timer_tick();
            } else if (irq == 1) {
                pic_send_eoi(1);
                Drivers::keyboard_irq();
            } else {
                pic_send_eoi(irq);
            }
        }
    }
}
