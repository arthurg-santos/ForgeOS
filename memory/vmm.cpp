#include "vmm.h"
#include "pmm.h"
#include "serial.h"

namespace Forge {
    namespace Memory {
        namespace {
            uint64_t* pml4 = nullptr;

            constexpr uint64_t PRESENT   = 1ULL << 0;
            constexpr uint64_t WRITABLE  = 1ULL << 1;
            constexpr uint64_t USER      = 1ULL << 2;
            constexpr uint64_t HUGE      = 1ULL << 7;
            constexpr uint64_t ADDR_MASK = 0x00007FFFFFFFF000ULL;

            int index_of(uint64_t va, int level) {
                return (int)((va >> (12 + 9 * level)) & 0x1FF);
            }

            uint64_t* table_of(uint64_t entry) {
                return (uint64_t*)(entry & ADDR_MASK);
            }

            uint64_t* alloc_table() {
                uint64_t phys = pmm_alloc_page();
                if (phys == 0) return nullptr;
                uint64_t* table = (uint64_t*)phys; // identity-mapped (< 1 GiB)
                for (int i = 0; i < 512; i++) table[i] = 0;
                return table;
            }

            void invlpg(uint64_t va) {
                __asm__ __volatile__("invlpg (%0)" : : "r"(va) : "memory");
            }
        }

        void vmm_init() {
            Interrupts::klog("vmm: allocating PML4");
            pml4 = alloc_table();
            if (pml4 == nullptr) {
                Interrupts::klog("vmm: FATAL - no page for PML4");
                return;
            }

            // Identity map do primeiro 1 GiB com huge pages de 2 MiB.
            // FASE 6: bit USER ativado neste ramo para permitir o primeiro
            // código em ring 3 (stacks e código de usuário vivem aqui).
            // O ramo higher-half (heap do kernel) permanece kernel-only.
            uint64_t* pdpt = alloc_table();
            uint64_t* pd   = alloc_table();
            for (int i = 0; i < 512; i++) {
                pd[i] = ((uint64_t)i * 0x200000ULL) | PRESENT | WRITABLE | HUGE | USER;
            }
            pdpt[0] = (uint64_t)pd | PRESENT | WRITABLE | USER;
            pml4[0] = (uint64_t)pdpt | PRESENT | WRITABLE | USER;

            // Higher-half (PML4[256] -> VA 0xFFFF800000000000+), kernel-only
            uint64_t* pdpt_high = alloc_table();
            pml4[256] = (uint64_t)pdpt_high | PRESENT | WRITABLE;

            uint64_t pml4_phys = (uint64_t)pml4;
            Interrupts::klog_hex("vmm: new PML4 phys", pml4_phys);
            __asm__ __volatile__("mov %0, %%cr3" : : "r"(pml4_phys) : "memory");
            Interrupts::klog("vmm: CR3 switched OK");
        }

        bool vmm_map_page(uint64_t va, uint64_t pa, uint64_t flags) {
            if (pml4 == nullptr) return false;

            uint64_t* table = pml4;
            for (int level = 3; level >= 1; level--) {
                int i = index_of(va, level);
                if (!(table[i] & PRESENT)) {
                    uint64_t* next = alloc_table();
                    if (next == nullptr) return false;
                    table[i] = (uint64_t)next | PRESENT | WRITABLE | (flags & VMM_USER);
                }
                table = table_of(table[i]);
            }

            int i = index_of(va, 0);
            if (table[i] & PRESENT) return false; // já mapeada
            table[i] = (pa & ADDR_MASK) | flags | PRESENT;
            invlpg(va);
            return true;
        }

        bool vmm_unmap_page(uint64_t va) {
            if (pml4 == nullptr) return false;

            uint64_t* table = pml4;
            for (int level = 3; level >= 1; level--) {
                int i = index_of(va, level);
                if (!(table[i] & PRESENT)) return false;
                table = table_of(table[i]);
            }

            int i = index_of(va, 0);
            if (!(table[i] & PRESENT)) return false;
            table[i] = 0;
            invlpg(va);
            return true;
        }

        uint64_t vmm_get_phys(uint64_t va) {
            if (pml4 == nullptr) return 0;

            uint64_t* table = pml4;
            for (int level = 3; level >= 1; level--) {
                int i = index_of(va, level);
                if (!(table[i] & PRESENT)) return 0;
                table = table_of(table[i]);
            }

            int i = index_of(va, 0);
            if (!(table[i] & PRESENT)) return 0;
            return (table[i] & ADDR_MASK) | (va & 0xFFF);
        }

        uint64_t vmm_current_pml4() { return (uint64_t)pml4; }
    }
}
