#ifndef IO_H
#define IO_H

#include "types.h"

namespace Forge {
    namespace Console {
        enum Color {
            Black = 0, Blue = 1, Green = 2, Cyan = 3, Red = 4, Magenta = 5,
            Brown = 6, LightGrey = 7, DarkGrey = 8, LightBlue = 9, LightGreen = 10,
            LightCyan = 11, LightRed = 12, LightMagenta = 13, LightBrown = 14, White = 15
        };

        void init();
        void clear();
        void set_color(Color fg, Color bg);
        void put_char(char c);
        void print(const char* str);
        void println(const char* str);
        void print_hex(uint64_t value);
        void print_dec(uint64_t value);
    }
}

#endif
