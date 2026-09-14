#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"
#include "mouse.h"

namespace Forge {
    namespace Interrupts {
        extern "C" void irq_handler(InterruptFrame* frame) {
            uint8_t irq = (uint8_t)(frame->int_no - 32);
            if (irq == 0) {
                pic_send_eoi(0);
                Drivers::timer_tick();
            } else if (irq == 1) {
                pic_send_eoi(1);
                Drivers::keyboard_irq();
            } else if (irq == 12) {
                pic_send_eoi(12);
                Drivers::mouse_irq();
            } else {
                pic_send_eoi(irq);
            }
        }
    }
}
