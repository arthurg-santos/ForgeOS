#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

namespace Forge {
    namespace Interrupts {
        void serial_init();
        void serial_write(char c);
        void serial_print(const char* str);
        void klog(const char* msg);
        void klog_hex(const char* msg, uint64_t value);
    }
}

#endif
