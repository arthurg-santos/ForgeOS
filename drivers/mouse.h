#ifndef MOUSE_H
#define MOUSE_H

#include "types.h"

namespace Forge {
    namespace Drivers {
        struct MouseEvent {
            int32_t dx, dy;
            uint8_t buttons; // bit0 = esquerdo
        };

        void mouse_init();
        void mouse_irq();
        bool mouse_get_event(MouseEvent* out);
    }
}

#endif
