#ifndef MULTIBOOT2_H
#define MULTIBOOT2_H

#include "types.h"

#define MULTIBOOT2_MAGIC          0x36D76289
#define MULTIBOOT2_TAG_END        0
#define MULTIBOOT2_TAG_MMAP       6
#define MULTIBOOT2_MEMORY_AVAILABLE 1

struct multiboot2_info {
    uint32_t total_size;
    uint32_t reserved;
} __attribute__((packed));

struct multiboot2_tag {
    uint32_t type;
    uint32_t size;
} __attribute__((packed));

struct multiboot2_mmap_tag {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} __attribute__((packed));

struct multiboot2_mmap_entry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t zero;
} __attribute__((packed));

#endif
