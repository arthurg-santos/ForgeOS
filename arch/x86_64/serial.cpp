#include "serial.h"
#include "io_ports.h"

#define COM1 0x3F8

namespace Forge {
    namespace Interrupts {
        void serial_init() {
            outb(COM1 + 1, 0x00);    // Desabilita interrupções da UART
            outb(COM1 + 3, 0x80);    // Habilita DLAB (divisor latch)
            outb(COM1 + 0, 0x01);    // Divisor low -> 115200 baud
            outb(COM1 + 1, 0x00);    // Divisor high
            outb(COM1 + 3, 0x03);    // 8 bits, sem paridade, 1 stop
            outb(COM1 + 2, 0xC7);    // Habilita FIFO, limpa, threshold 14
            outb(COM1 + 4, 0x0B);    // RTS/DSR set
        }

        void serial_write(char c) {
            while ((inb(COM1 + 5) & 0x20) == 0) { }
            outb(COM1, c);
        }

        void serial_print(const char* str) {
            for (size_t i = 0; str[i] != '\0'; i++) {
                if (str[i] == '\n') serial_write('\r');
                serial_write(str[i]);
            }
        }

        void klog(const char* msg) {
            serial_print("[klog] ");
            serial_print(msg);
            serial_print("\n");
        }

        void klog_hex(const char* msg, uint64_t value) {
            const char* hex = "0123456789ABCDEF";
            char buf[17];
            buf[16] = '\0';
            for (int i = 15; i >= 0; i--) {
                buf[i] = hex[value & 0xF];
                value >>= 4;
            }
            serial_print("[klog] ");
            serial_print(msg);
            serial_print(" = 0x");
            serial_print(buf);
            serial_print("\n");
        }
    }
}
