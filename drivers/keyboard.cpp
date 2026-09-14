#include "keyboard.h"
#include "io_ports.h"
#include "pic.h"
#include "serial.h"

namespace Forge {
    namespace Drivers {
        namespace {
            constexpr uint16_t KBD_DATA = 0x60;

            char ring[256];
            volatile uint64_t head = 0;
            volatile uint64_t tail = 0;
            bool shift = false;
            bool e0 = false;

            const char map_norm[128] = {
                /* 0x00 */ 0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', 0,
                /* 0x10 */ 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
                /* 0x20 */ 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
                /* 0x30 */ 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
                /* 0x40 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                /* 0x50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                /* 0x60 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                /* 0x70 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            };
            const char map_shift[128] = {
                /* 0x00 */ 0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', 0,
                /* 0x10 */ 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S',
                /* 0x20 */ 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
                /* 0x30 */ 'B', 'N', 'M', '<', '>', '?', 0, 0, 0, ' ', 0, 0, 0, 0, 0, 0,
                /* 0x40 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                /* 0x50 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                /* 0x60 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                /* 0x70 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            };

            void push_char(char c) {
                if (head - tail < 256) {
                    ring[head & 255] = c;
                    head = head + 1;
                }
            }
        }

        void keyboard_init() {
            Interrupts::pic_unmask(1);
            Interrupts::klog("kbd: PS/2 keyboard online (IRQ1)");
        }

        void keyboard_irq() {
            uint8_t sc = inb(KBD_DATA);

            if (sc == 0xE0) { e0 = true; return; }

            if (sc & 0x80) {
                if (sc == 0xAA || sc == 0xB6) shift = false;
                e0 = false;
                return;
            }

            if (sc == 0x01) { push_char(8); return; } // ESC = código especial 8

            if (e0) {
                e0 = false;
                char special = 0;
                switch (sc) {
                    case 0x48: special = 1; break;
                    case 0x50: special = 2; break;
                    case 0x4B: special = 3; break;
                    case 0x4D: special = 4; break;
                    case 0x47: special = 5; break;
                    case 0x4F: special = 6; break;
                    case 0x53: special = 7; break;
                    default: return;
                }
                push_char(special);
                return;
            }

            if (sc == 0x2A || sc == 0x36) { shift = true; return; }

            char c = shift ? map_shift[sc] : map_norm[sc];
            if (c != 0) push_char(c);
        }

        int keyboard_get_char() {
            if (head == tail) return -1;
            char c = ring[tail & 255];
            tail = tail + 1;
            return (int)(unsigned char)c;
        }
    }
}
