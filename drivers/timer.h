#ifndef TIMER_H
#define TIMER_H

#include "types.h"

namespace Forge {
    namespace Drivers {
        void timer_init(uint32_t frequency_hz);
        void timer_tick();
        uint64_t timer_ticks();
    }
}

#endif
