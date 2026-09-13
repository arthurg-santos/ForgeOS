#include "gdt.h"
#include "io_ports.h"
#include "serial.h"

extern "C" void gdt_flush(uint64_t gdt_ptr_addr);
extern "C" void tss_flush(uint16_t selector);

namespace Forge {
    namespace CPU {
        struct GDTEntry {
            uint16_t limit_low;
            uint16_t base_low;
            uint8_t base_middle;
            uint8_t access;
            uint8_t granularity;
            uint8_t base_high;
        } __attribute__((packed));

        struct TSSEntry {
            uint16_t length;
            uint16_t base_low;
            uint8_t base_mid;
            uint8_t flags1;
            uint8_t flags2;
            uint8_t base_high;
            uint32_t base_upper;
            uint32_t reserved;
        } __attribute__((packed));

        struct TSS {
            uint32_t reserved0;
            uint64_t rsp0;
            uint64_t rsp1;
            uint64_t rsp2;
            uint64_t reserved1;
            uint64_t ist1;
            uint64_t ist2;
            uint64_t ist3;
            uint64_t ist4;
            uint64_t ist5;
            uint64_t ist6;
            uint64_t ist7;
            uint64_t reserved2;
            uint16_t reserved3;
            uint16_t iomap_base;
        } __attribute__((packed));

        TSS tss __attribute__((aligned(16)));
        GDTEntry gdt[7];

        struct {
            uint16_t limit;
            uint64_t base;
        } __attribute__((packed)) gdt_ptr;

        void gdt_init() {
            Interrupts::klog("gdt: zeroing TSS");

            for (size_t i = 0; i < sizeof(TSS); i++) {
                ((uint8_t*)&tss)[i] = 0;
            }
            tss.iomap_base = sizeof(TSS);

            uint64_t tss_base = (uint64_t)&tss;
            uint32_t tss_limit = sizeof(TSS) - 1;

            Interrupts::klog("gdt: filling descriptors");

            gdt[0] = {0, 0, 0, 0x00, 0x00, 0}; // Null
            gdt[1] = {0, 0, 0, 0x9A, 0x20, 0}; // Kernel Code  0x08
            gdt[2] = {0, 0, 0, 0x92, 0x00, 0}; // Kernel Data  0x10
            // gdt[3..4] = TSS (0x18), preenchido abaixo
            gdt[5] = {0, 0, 0, 0xFA, 0x20, 0}; // User Code  0x28 (DPL=3)
            gdt[6] = {0, 0, 0, 0xF2, 0x00, 0}; // User Data  0x30 (DPL=3)

            TSSEntry* tss_entry = (TSSEntry*)&gdt[3];
            tss_entry->length = tss_limit;
            tss_entry->base_low = tss_base & 0xFFFF;
            tss_entry->base_mid = (tss_base >> 16) & 0xFF;
            tss_entry->flags1 = 0x89;
            tss_entry->flags2 = 0x00;
            tss_entry->base_high = (tss_base >> 24) & 0xFF;
            tss_entry->base_upper = (tss_base >> 32) & 0xFFFFFFFF;
            tss_entry->reserved = 0;

            gdt_ptr.limit = (sizeof(GDTEntry) * 7) - 1;
            gdt_ptr.base = (uint64_t)&gdt;

            Interrupts::klog("gdt: calling gdt_flush (lgdt+reload)");
            gdt_flush((uint64_t)&gdt_ptr);
            Interrupts::klog("gdt: gdt_flush returned OK");

            tss_flush(0x18);
            Interrupts::klog("gdt: tss_flush (ltr) OK");
        }

        void set_tss_rsp0(uint64_t rsp0) {
            tss.rsp0 = rsp0;
        }
    }
}
