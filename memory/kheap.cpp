#include "kheap.h"
#include "vmm.h"
#include "pmm.h"
#include "serial.h"

namespace Forge {
    namespace Memory {
        namespace {
            constexpr uint64_t HEAP_START = 0xFFFF800000000000ULL;
            constexpr uint64_t HEAP_INITIAL_BYTES = 64 * 1024;   // 64 KiB iniciais
            constexpr uint64_t HEAP_MAX_BYTES = 4 * 1024 * 1024; // teto de 4 MiB
            constexpr uint64_t HDR = 32; // header de bloco (múltiplo de 16)

            struct BlockHeader {
                uint64_t size;      // tamanho do payload
                uint64_t free;      // 1 = livre, 0 = em uso
                BlockHeader* next;  // próximo bloco (lista ordenada por endereço)
                uint64_t reserved;
            };

            BlockHeader* heap_head = nullptr;
            uint64_t heap_end = 0;
            uint64_t used_bytes = 0;

            uint64_t align16(uint64_t v) { return (v + 15ULL) & ~15ULL; }

            uint8_t* block_end(BlockHeader* b) {
                return (uint8_t*)b + HDR + b->size;
            }

            // Mapeia novas páginas físicas no fim do heap virtual.
            // Retorna quantos bytes foram adicionados (0 em falha).
            uint64_t heap_expand(uint64_t bytes) {
                uint64_t pages = (bytes + PAGE_SIZE - 1) / PAGE_SIZE;
                uint64_t added = 0;
                for (uint64_t i = 0; i < pages; i++) {
                    if (heap_end - HEAP_START >= HEAP_MAX_BYTES) break;
                    uint64_t phys = pmm_alloc_page();
                    if (phys == 0) break;
                    if (!vmm_map_page(heap_end, phys, VMM_PRESENT | VMM_WRITABLE)) {
                        pmm_free_page(phys);
                        break;
                    }
                    heap_end += PAGE_SIZE;
                    added += PAGE_SIZE;
                }
                return added;
            }

            // Divide o bloco: [HDR|size][novo bloco com o restante]
            void insert_split(BlockHeader* block, uint64_t size) {
                BlockHeader* new_block = (BlockHeader*)((uint8_t*)block + HDR + size);
                new_block->size = block->size - size - HDR;
                new_block->free = 1;
                new_block->next = block->next;
                new_block->reserved = 0;
                block->size = size;
                block->next = new_block;
            }
        }

        void kheap_init() {
            heap_end = HEAP_START;
            uint64_t added = heap_expand(HEAP_INITIAL_BYTES);
            if (added < HEAP_INITIAL_BYTES) {
                Interrupts::klog("kheap: FATAL - cannot map initial heap");
                return;
            }
            heap_head = (BlockHeader*)HEAP_START;
            heap_head->size = added - HDR;
            heap_head->free = 1;
            heap_head->next = nullptr;
            heap_head->reserved = 0;
            used_bytes = 0;
            Interrupts::klog_hex("kheap: heap start", HEAP_START);
        }

        void* kmalloc(uint64_t size) {
            if (size == 0 || heap_head == nullptr) return nullptr;
            size = align16(size);

            // First-fit na lista de blocos
            for (BlockHeader* b = heap_head; b != nullptr; b = b->next) {
                if (b->free && b->size >= size) {
                    if (b->size >= size + HDR + 16) insert_split(b, size);
                    b->free = 0;
                    used_bytes += b->size;
                    return (void*)((uint8_t*)b + HDR);
                }
            }

            // Sem bloco adequado: cresce o último bloco, se ele terminar
            // exatamente no fim do heap virtual mapeado.
            BlockHeader* last = heap_head;
            while (last->next != nullptr) last = last->next;
            if (last->free && (uint64_t)block_end(last) == heap_end) {
                uint64_t need = size + HDR - last->size;
                uint64_t added = heap_expand(need);
                if (added == 0) return nullptr;
                last->size += added;
                if (last->size >= size + HDR + 16) insert_split(last, size);
                last->free = 0;
                used_bytes += last->size;
                return (void*)((uint8_t*)last + HDR);
            }
            return nullptr;
        }

        void kfree(void* ptr) {
            if (ptr == nullptr) return;
            BlockHeader* b = (BlockHeader*)((uint8_t*)ptr - HDR);
            if (b->free) return; // double free: ignora silenciosamente
            b->free = 1;
            used_bytes -= b->size;

            // Coalesce com o próximo bloco, se livre e contíguo
            if (b->next != nullptr && b->next->free &&
                (uint64_t)block_end(b) == (uint64_t)b->next) {
                b->size += HDR + b->next->size;
            b->next = b->next->next;
                }

                // Coalesce com o bloco anterior, se livre e contíguo
                BlockHeader* prev = heap_head;
                while (prev != nullptr && prev->next != b) prev = prev->next;
                if (prev != nullptr && prev->free &&
                    (uint64_t)block_end(prev) == (uint64_t)b) {
                    prev->size += HDR + b->size;
                prev->next = b->next;
                    }
        }

        uint64_t kheap_used() { return used_bytes; }
        uint64_t kheap_capacity() { return heap_end - HEAP_START; }
    }
}
