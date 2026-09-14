#include "mouse.h"
#include "io_ports.h"
#include "pic.h"
#include "serial.h"

namespace Forge {
    namespace Drivers {
        namespace {
            constexpr uint16_t DATA = 0x60;
            constexpr uint16_t CMD  = 0x64;

            MouseEvent ring[32];
            volatile uint64_t head = 0;
            volatile uint64_t tail = 0;

            uint8_t packet[3];
            int phase = 0;
            int logged = 0;

            void wait_write() {
                for (int i = 0; i < 100000; i++) {
                    if (!(inb(CMD) & 0x02)) return;
                }
            }
            void wait_read() {
                for (int i = 0; i < 100000; i++) {
                    if (inb(CMD) & 0x01) return;
                }
            }

            // Controller Command Byte do 8042: o cadeado que faltava.
            uint8_t ccb_read() {
                wait_write();
                outb(CMD, 0x20);
                wait_read();
                return inb(DATA);
            }
            void ccb_write(uint8_t v) {
                wait_write();
                outb(CMD, 0x60);
                wait_write();
                outb(DATA, v);
            }

            // Comando ao mouse (prefixo 0xD4). Não lemos o ACK aqui: em
            // contexto com IRQs ligadas, o handler de teclado poderia roubar
            // o byte do buffer auxiliar e dessincronizar o handshake.
            void mouse_cmd(uint8_t b) {
                wait_write();
                outb(CMD, 0xD4);
                wait_write();
                outb(DATA, b);
            }
        }

        void mouse_init() {
            wait_write();
            outb(CMD, 0xA8); // enable auxiliary port

            uint8_t ccb = ccb_read();
            ccb |= 0x02;     // bit1 = 1: habilita IRQ do mouse (IRQ12)
            ccb &= ~0x20;    // bit5 = 0: libera o clock do mouse
            ccb &= ~0x40;    // bit6 = 0: sem tradução (pacotes raw)
            ccb_write(ccb);

            mouse_cmd(0xF6); // defaults
            mouse_cmd(0xF4); // enable data reporting

            Interrupts::pic_unmask(2);  // cascade do slave
            Interrupts::pic_unmask(12); // IRQ12
            Interrupts::klog("mouse: online (IRQ12, CCB corrigido)");
        }

        void mouse_irq() {
            uint8_t status = inb(CMD);
            if (!(status & 0x20)) return; // buffer aux vazio
            uint8_t b = inb(DATA);

            if (phase == 0) {
                if (!(b & 0x08)) return; // bit de sincronia
                packet[0] = b;
                phase = 1;
            } else if (phase == 1) {
                packet[1] = b;
                phase = 2;
            } else {
                packet[2] = b;
                phase = 0;
                MouseEvent ev;
                ev.dx = (int32_t)(int8_t)packet[1];
                ev.dy = (int32_t)(int8_t)packet[2];
                ev.buttons = packet[0] & 0x07;
                if (head - tail < 32) {
                    ring[head & 31] = ev;
                    head = head + 1;
                }
                if (logged < 3) {
                    Interrupts::klog_hex("mouse: primeiro evento, buttons", ev.buttons);
                    logged = logged + 1;
                }
            }
        }

        bool mouse_get_event(MouseEvent* out) {
            if (head == tail) return false;
            *out = ring[tail & 31];
            tail = tail + 1;
            return true;
        }
    }
}
