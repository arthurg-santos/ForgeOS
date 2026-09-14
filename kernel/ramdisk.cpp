#include "ramdisk.h"
#include "serial.h"

namespace Forge {
    namespace Storage {
        namespace {
            RamdiskEntry entries[RD_MAX_FILES];

            void copy_name(char* dst, const char* src) {
                int i = 0;
                while (src[i] && i < RD_MAX_NAME - 1) { dst[i] = src[i]; i++; }
                dst[i] = '\0';
            }

            int name_eq(const char* a, const char* b) {
                int i = 0;
                while (a[i] && b[i] && a[i] == b[i]) i++;
                return a[i] == b[i];
            }
        }

        void ramdisk_init() {
            for (int i = 0; i < RD_MAX_FILES; i++) {
                entries[i].used = false;
                entries[i].size = 0;
                entries[i].name[0] = '\0';
            }
            Interrupts::klog("ramdisk: initialized (32 slots x 4 KiB)");
        }

        int rd_find(const char* name) {
            for (int i = 0; i < RD_MAX_FILES; i++) {
                if (entries[i].used && name_eq(entries[i].name, name)) return i;
            }
            return -1;
        }

        int rd_create(const char* name) {
            if (rd_find(name) >= 0) return -2; // já existe
            for (int i = 0; i < RD_MAX_FILES; i++) {
                if (!entries[i].used) {
                    entries[i].used = true;
                    entries[i].size = 0;
                    copy_name(entries[i].name, name);
                    return i;
                }
            }
            return -1; // cheio
        }

        int rd_delete(const char* name) {
            int i = rd_find(name);
            if (i < 0) return -1;
            entries[i].used = false;
            entries[i].size = 0;
            entries[i].name[0] = '\0';
            return 0;
        }

        int rd_read(int idx, uint32_t off, uint8_t* buf, uint32_t n) {
            if (idx < 0 || idx >= RD_MAX_FILES || !entries[idx].used) return -1;
            if (off > entries[idx].size) return 0;
            uint32_t avail = entries[idx].size - off;
            if (n > avail) n = avail;
            for (uint32_t k = 0; k < n; k++) buf[k] = entries[idx].data[off + k];
            return (int)n;
        }

        int rd_write(int idx, uint32_t off, const uint8_t* buf, uint32_t n) {
            if (idx < 0 || idx >= RD_MAX_FILES || !entries[idx].used) return -1;
            if (off + n > RD_MAX_SIZE) n = RD_MAX_SIZE - off;
            for (uint32_t k = 0; k < n; k++) entries[idx].data[off + k] = buf[k];
            if (off + n > entries[idx].size) entries[idx].size = off + n;
            return (int)n;
        }

        int rd_size(int idx) {
            if (idx < 0 || idx >= RD_MAX_FILES || !entries[idx].used) return -1;
            return (int)entries[idx].size;
        }

        bool rd_list(int i, char* name_out, uint32_t* size_out) {
            if (i < 0 || i >= RD_MAX_FILES) return false;
            if (!entries[i].used) return false;
            copy_name(name_out, entries[i].name);
            *size_out = entries[i].size;
            return true;
        }
    }
}
