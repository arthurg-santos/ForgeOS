#ifndef PIC_H
#define PIC_H

#include "types.h"

namespace Forge {
    namespace Interrupts {
        void pic_init();
        void pic_send_eoi(uint8_t irq);
        void pic_unmask(uint8_t irq);
        void pic_mask(uint8_t irq);
    }
}

#endif
