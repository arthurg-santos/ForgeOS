#ifndef VFS_H
#define VFS_H

#include "types.h"

namespace Forge {
    namespace VFS {
        constexpr int FD_MAX = 16;

        constexpr int O_RDONLY = 0;
        constexpr int O_WRONLY = 1;
        constexpr int O_RDWR   = 2;
        constexpr int O_CREATE = 4;

        void vfs_init();
        int vfs_open(const char* name, int flags);
        int vfs_read(int fd, uint8_t* buf, uint32_t n);
        int vfs_write(int fd, const uint8_t* buf, uint32_t n);
        int vfs_close(int fd);
        int vfs_ls(char* buf, uint32_t maxlen);
        int vfs_rm(const char* name);
    }
}

#endif
