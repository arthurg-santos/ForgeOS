#include "timer.h"
#include "io_ports.h"
#include "pic.h"
#include "scheduler.h"
#include "serial.h"

namespace Forge {
    namespace Drivers {
        namespace {
            volatile uint64_t ticks = 0;
            constexpr uint64_t TICKS_PER_SCHEDULE = 10; // troca de task a cada 10 ticks
        }

        void timer_init(uint32_t frequency_hz) {
            // PIT: base clock 1193182 Hz, canal 0, modo 3 (square wave)
            uint32_t divisor = 1193182 / frequency_hz;
            outb(0x43, 0x36);
            outb(0x40, (uint8_t)(divisor & 0xFF));
            outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
            Interrupts::pic_unmask(0); // IRQ0 = timer
            Interrupts::klog_hex("timer: PIT programmed at Hz", frequency_hz);
        }

        void timer_tick() {
            ticks++;
            if (ticks % TICKS_PER_SCHEDULE == 0) {
                Kernel::schedule(); // preempção por timer
            }
        }

        uint64_t timer_ticks() { return ticks; }
    }
}
