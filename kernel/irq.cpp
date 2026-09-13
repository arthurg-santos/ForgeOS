#include "idt.h"
#include "pic.h"
#include "timer.h"

namespace Forge {
    namespace Interrupts {
        extern "C" void irq_handler(InterruptFrame* frame) {
            uint8_t irq = (uint8_t)(frame->int_no - 32);
            if (irq == 0) {
                // EOI ANTES de qualquer troca de contexto: caso o timer
                // troque de task aqui, o PIC já está liberado para a próxima.
                pic_send_eoi(0);
                Drivers::timer_tick();
            } else {
                pic_send_eoi(irq);
            }
        }
    }
}
