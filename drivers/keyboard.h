#ifndef KEYBOARD_H
#define KEYBOARD_H

namespace Forge {
    namespace Drivers {
        void keyboard_init();
        void keyboard_irq();
        int keyboard_get_char(); // -1 se vazio
    }
}

#endif
