#include "io.h"

namespace Forge {
    namespace Console {
        volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;
        const size_t VGA_WIDTH = 80;
        const size_t VGA_HEIGHT = 25;

        size_t terminal_row;
        size_t terminal_column;
        uint8_t terminal_color;

        inline uint16_t vga_entry(char uc, uint8_t color) {
            uint16_t c = (uint8_t)uc;
            uint16_t col = color;
            return c | (col << 8);
        }

        void init() {
            terminal_row = 0;
            terminal_column = 0;
            terminal_color = LightGrey | (Black << 4);
            clear();
        }

        void clear() {
            for (size_t y = 0; y < VGA_HEIGHT; y++) {
                for (size_t x = 0; x < VGA_WIDTH; x++) {
                    size_t index = y * VGA_WIDTH + x;
                    vga_buffer[index] = vga_entry(' ', terminal_color);
                }
            }
            terminal_row = 0;
            terminal_column = 0;
        }

        void set_color(Color fg, Color bg) {
            terminal_color = fg | (bg << 4);
        }

        void put_char(char c) {
            if (c == '\n') {
                terminal_row++;
                terminal_column = 0;
                if (terminal_row >= VGA_HEIGHT) {
                    terminal_row = 0;
                    clear();
                }
                return;
            }

            if (terminal_column >= VGA_WIDTH) {
                terminal_column = 0;
                terminal_row++;
                if (terminal_row >= VGA_HEIGHT) {
                    terminal_row = 0;
                    clear();
                }
            }

            size_t index = terminal_row * VGA_WIDTH + terminal_column;
            vga_buffer[index] = vga_entry(c, terminal_color);
            terminal_column++;
        }

        void print(const char* str) {
            for (size_t i = 0; str[i] != '\0'; i++) {
                put_char(str[i]);
            }
        }

        void println(const char* str) {
            print(str);
            put_char('\n');
        }

        void print_hex(uint64_t value) {
            print("0x");
            const char hex_chars[] = "0123456789ABCDEF";
            char buffer[17];
            buffer[16] = '\0';
            for (int i = 15; i >= 0; i--) {
                buffer[i] = hex_chars[value & 0xF];
                value >>= 4;
            }
            print(buffer);
        }

        void print_dec(uint64_t value) {
            char buffer[21];
            buffer[20] = '\0';
            int i = 19;
            if (value == 0) {
                put_char('0');
                return;
            }
            while (value > 0) {
                buffer[i--] = '0' + (value % 10);
                value /= 10;
            }
            print(&buffer[i + 1]);
        }
    }
}
