#ifndef RAMDISK_H
#define RAMDISK_H

#include "types.h"

namespace Forge {
    namespace Storage {
        constexpr int RD_MAX_FILES = 32;
        constexpr int RD_MAX_NAME  = 32;
        constexpr int RD_MAX_SIZE  = 4096;

        struct RamdiskEntry {
            char name[RD_MAX_NAME];
            uint32_t size;
            uint8_t data[RD_MAX_SIZE];
            bool used;
        };

        void ramdisk_init();
        int rd_find(const char* name);
        int rd_create(const char* name);
        int rd_delete(const char* name);
        int rd_read(int idx, uint32_t off, uint8_t* buf, uint32_t n);
        int rd_write(int idx, uint32_t off, const uint8_t* buf, uint32_t n);
        int rd_size(int idx);
        bool rd_list(int i, char* name_out, uint32_t* size_out); // false se i fora
    }
}

#endif
