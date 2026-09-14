#include "vfs.h"
#include "ramdisk.h"
#include "serial.h"

namespace Forge {
    namespace VFS {
        namespace {
            struct FD {
                bool open;
                int rd_idx;
                int flags;
                uint32_t offset;
            };
            FD table[FD_MAX];
        }

        void vfs_init() {
            for (int i = 0; i < FD_MAX; i++) table[i].open = false;
            Interrupts::klog("vfs: initialized (16 FDs)");
        }

        int vfs_open(const char* name, int flags) {
            int idx = Storage::rd_find(name);
            if (idx < 0) {
                if (!(flags & O_CREATE)) return -1;
                idx = Storage::rd_create(name);
                if (idx < 0) return -1;
            }
            for (int i = 0; i < FD_MAX; i++) {
                if (!table[i].open) {
                    table[i].open = true;
                    table[i].rd_idx = idx;
                    table[i].flags = flags;
                    table[i].offset = 0;
                    return i;
                }
            }
            return -1;
        }

        int vfs_read(int fd, uint8_t* buf, uint32_t n) {
            if (fd < 0 || fd >= FD_MAX || !table[fd].open) return -1;
            int got = Storage::rd_read(table[fd].rd_idx, table[fd].offset, buf, n);
            if (got > 0) table[fd].offset += (uint32_t)got;
            return got;
        }

        int vfs_write(int fd, const uint8_t* buf, uint32_t n) {
            if (fd < 0 || fd >= FD_MAX || !table[fd].open) return -1;
            if (table[fd].flags == O_RDONLY) return -1;
            int wrote = Storage::rd_write(table[fd].rd_idx, table[fd].offset, buf, n);
            if (wrote > 0) table[fd].offset += (uint32_t)wrote;
            return wrote;
        }

        int vfs_close(int fd) {
            if (fd < 0 || fd >= FD_MAX || !table[fd].open) return -1;
            table[fd].open = false;
            return 0;
        }

        int vfs_ls(char* buf, uint32_t maxlen) {
            uint32_t used = 0;
            int count = 0;
            for (int i = 0; i < Storage::RD_MAX_FILES; i++) {
                char name[Storage::RD_MAX_NAME];
                uint32_t size;
                if (!Storage::rd_list(i, name, &size)) continue;

                uint32_t nlen = 0; while (name[nlen]) nlen++;
                // formato: "name\tsize\n"
                char sizebuf[16];
                int si = 0;
                uint32_t v = size;
                if (v == 0) sizebuf[si++] = '0';
                else {
                    char t[16]; int ti = 0;
                    while (v > 0) { t[ti++] = (char)('0' + (v % 10)); v /= 10; }
                    while (ti > 0) sizebuf[si++] = t[--ti];
                }
                sizebuf[si] = '\0';

                uint32_t need = nlen + 1 + (uint32_t)si + 1; // name \t size \n
                if (used + need + 1 > maxlen) break;
                for (uint32_t k = 0; k < nlen; k++) buf[used++] = name[k];
                buf[used++] = '\t';
                for (int k = 0; k < si; k++) buf[used++] = sizebuf[k];
                buf[used++] = '\n';
                count++;
            }
            buf[used] = '\0';
            return count;
        }

        int vfs_rm(const char* name) {
            return Storage::rd_delete(name);
        }
    }
}
