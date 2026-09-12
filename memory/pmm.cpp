#include "pmm.h"
#include "multiboot2.h"
#include "serial.h"

extern "C" uint8_t kernel_end[];

namespace Forge {
    namespace Memory {
        namespace {
            constexpr uint64_t MAX_PAGES = 1024 * 1024;      // Bitmap cobre até 4 GB
            constexpr uint64_t BITMAP_BYTES = MAX_PAGES / 8; // 128 KiB de bitmap
            constexpr uint64_t LOWER_RESERVE = 0x100000;     // Primeiro 1 MiB

            uint8_t page_bitmap[BITMAP_BYTES];
            uint64_t total_pages = 0;
            uint64_t free_pages = 0;
            uint64_t highest_page = 0;

            void bitmap_set(uint64_t page)   { page_bitmap[page / 8] |=  (uint8_t)(1 << (page % 8)); }
            void bitmap_clear(uint64_t page) { page_bitmap[page / 8] &= (uint8_t)~(1 << (page % 8)); }
            bool bitmap_test(uint64_t page)  { return (page_bitmap[page / 8] & (uint8_t)(1 << (page % 8))) != 0; }

            // Libera [base, base+length) no bitmap. Chamado apenas para
            // regiões de RAM disponível reportadas pelo Multiboot2.
            void free_region(uint64_t base, uint64_t length) {
                uint64_t start = (base + PAGE_SIZE - 1) / PAGE_SIZE; // arredonda p/ cima
                uint64_t end   = (base + length) / PAGE_SIZE;        // arredonda p/ baixo
                if (end > MAX_PAGES) end = MAX_PAGES;
                for (uint64_t p = start; p < end; p++) {
                    bitmap_clear(p);
                    total_pages++;
                }
                if (end > highest_page) highest_page = end;
            }

            // Marca [base, base+length) como usado (arredondamento conservador).
            void reserve_region(uint64_t base, uint64_t length) {
                uint64_t start = base / PAGE_SIZE;
                uint64_t end   = (base + length + PAGE_SIZE - 1) / PAGE_SIZE;
                if (end > MAX_PAGES) end = MAX_PAGES;
                for (uint64_t p = start; p < end; p++) bitmap_set(p);
            }
        }

        void pmm_init(uint64_t mb_addr) {
            // Começa com TUDO marcado como usado; só libera o que o
            // memory map garantir que é RAM disponível.
            for (uint64_t i = 0; i < BITMAP_BYTES; i++) page_bitmap[i] = 0xFF;
            total_pages = 0;
            free_pages = 0;
            highest_page = 0;

            if (mb_addr != 0) {
                const multiboot2_info* info = (const multiboot2_info*)mb_addr;
                uint32_t total_size = info->total_size;
                uint64_t pos = mb_addr + 8;

                while (pos + 8 <= mb_addr + total_size) {
                    const multiboot2_tag* tag = (const multiboot2_tag*)pos;
                    if (tag->type == MULTIBOOT2_TAG_END) break;

                    if (tag->type == MULTIBOOT2_TAG_MMAP) {
                        const multiboot2_mmap_tag* mmap = (const multiboot2_mmap_tag*)pos;
                        uint64_t entry_pos = pos + sizeof(multiboot2_mmap_tag);
                        uint64_t entries_end = pos + mmap->size;

                        while (entry_pos + mmap->entry_size <= entries_end) {
                            const multiboot2_mmap_entry* e = (const multiboot2_mmap_entry*)entry_pos;
                            if (e->type == MULTIBOOT2_MEMORY_AVAILABLE) {
                                free_region(e->base_addr, e->length);
                            }
                            entry_pos += mmap->entry_size;
                        }
                    }

                    pos += ((uint64_t)tag->size + 7) & ~(uint64_t)7; // tags alinhadas em 8
                }
            }

            // Reservas obrigatórias:
            reserve_region(0, LOWER_RESERVE);                                  // 1º MiB (BIOS/legacy)
            reserve_region(LOWER_RESERVE, (uint64_t)kernel_end - LOWER_RESERVE); // imagem do kernel
            if (mb_addr >= LOWER_RESERVE) {                                    // estrutura Multiboot2
                reserve_region(mb_addr, ((const multiboot2_info*)mb_addr)->total_size);
            }

            // Conta páginas livres após as reservas
            free_pages = 0;
            for (uint64_t p = 0; p < highest_page; p++) {
                if (!bitmap_test(p)) free_pages++;
            }

            Interrupts::klog_hex("pmm: kernel_end", (uint64_t)kernel_end);
            Interrupts::klog_hex("pmm: mb_addr", mb_addr);
            Interrupts::klog_hex("pmm: highest_page", highest_page);
        }

        uint64_t pmm_alloc_page() {
            for (uint64_t p = 0; p < highest_page; p++) {
                // Atalho: byte cheio (0xFF) = 8 páginas usadas, pula de uma vez
                if ((p % 8) == 0 && page_bitmap[p / 8] == 0xFF) {
                    p += 7;
                    continue;
                }
                if (!bitmap_test(p)) {
                    bitmap_set(p);
                    free_pages--;
                    return p * PAGE_SIZE;
                }
            }
            return 0; // sem memória
        }

        void pmm_free_page(uint64_t addr) {
            uint64_t p = addr / PAGE_SIZE;
            if (p < highest_page && bitmap_test(p)) {
                bitmap_clear(p);
                free_pages++;
            }
        }

        uint64_t pmm_total_pages() { return total_pages; }
        uint64_t pmm_free_pages()  { return free_pages; }
    }
}
