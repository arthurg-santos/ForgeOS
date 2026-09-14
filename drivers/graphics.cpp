#include "graphics.h"
#include "io_ports.h"
#include "vmm.h"
#include "kheap.h"
#include "serial.h"

namespace Forge {
    namespace Graphics {
        namespace {
            FBInfo fb = {0, 0, 0, 0, 0, false};
            uint32_t* back = nullptr; // backbuffer offscreen (double buffering)

            inline uint32_t inl(uint16_t port) {
                uint32_t v;
                __asm__ __volatile__("inl %1, %0" : "=a"(v) : "Nd"(port));
                return v;
            }
            inline void outl(uint16_t port, uint32_t v) {
                __asm__ __volatile__("outl %0, %1" : : "a"(v), "Nd"(port));
            }
            inline void outw(uint16_t port, uint16_t v) {
                __asm__ __volatile__("outw %0, %1" : : "a"(v), "Nd"(port));
            }

            uint32_t pci_read(uint8_t slot, uint8_t offset) {
                outl(0xCF8, 0x80000000u | ((uint32_t)slot << 11) | (offset & 0xFC));
                return inl(0xCFC);
            }

            uint64_t find_vga_bar0() {
                for (uint8_t slot = 0; slot < 32; slot++) {
                    uint32_t id = pci_read(slot, 0x00);
                    if (id == 0xFFFFFFFF || id == 0) continue;
                    uint32_t cls = pci_read(slot, 0x08);
                    if ((cls >> 24) == 0x03) {
                        uint32_t bar0 = pci_read(slot, 0x10) & ~0xFu;
                        if (bar0 != 0) return bar0;
                    }
                }
                return 0;
            }

            constexpr uint16_t VBE_INDEX = 0x1CE;
            constexpr uint16_t VBE_DATA  = 0x1CF;
            void vbe_write(uint16_t reg, uint16_t val) {
                outw(VBE_INDEX, reg);
                outw(VBE_DATA, val);
            }

            struct FG { char c; uint8_t r[8]; };
            const FG FONT[] = {
                {' ', {0,0,0,0,0,0,0,0}},
                {'+', {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}},
                {'A', {0x18,0x24,0x42,0x42,0x7E,0x42,0x42,0x42}},
                {'B', {0x7C,0x42,0x42,0x7C,0x42,0x42,0x42,0x7C}},
                {'C', {0x3C,0x42,0x40,0x40,0x40,0x40,0x42,0x3C}},
                {'D', {0x78,0x44,0x42,0x42,0x42,0x42,0x44,0x78}},
                {'E', {0x7E,0x40,0x40,0x7C,0x40,0x40,0x40,0x7E}},
                {'F', {0x7E,0x40,0x40,0x7C,0x40,0x40,0x40,0x40}},
                {'G', {0x3C,0x42,0x40,0x40,0x4E,0x42,0x42,0x3C}},
                {'H', {0x42,0x42,0x42,0x7E,0x42,0x42,0x42,0x42}},
                {'I', {0x3E,0x08,0x08,0x08,0x08,0x08,0x08,0x3E}},
                {'J', {0x1E,0x04,0x04,0x04,0x04,0x04,0x44,0x38}},
                {'K', {0x42,0x44,0x48,0x70,0x48,0x44,0x42,0x42}},
                {'L', {0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x7E}},
                {'M', {0x42,0x66,0x5A,0x42,0x42,0x42,0x42,0x42}},
                {'N', {0x42,0x62,0x52,0x4A,0x46,0x42,0x42,0x42}},
                {'O', {0x3C,0x42,0x42,0x42,0x42,0x42,0x42,0x3C}},
                {'P', {0x7C,0x42,0x42,0x7C,0x40,0x40,0x40,0x40}},
                {'Q', {0x3C,0x42,0x42,0x42,0x42,0x52,0x4A,0x3C}},
                {'R', {0x7C,0x42,0x42,0x7C,0x48,0x44,0x42,0x42}},
                {'S', {0x3C,0x42,0x40,0x3C,0x02,0x42,0x42,0x3C}},
                {'T', {0x7F,0x08,0x08,0x08,0x08,0x08,0x08,0x08}},
                {'U', {0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x3C}},
                {'V', {0x42,0x42,0x42,0x42,0x42,0x24,0x24,0x18}},
                {'W', {0x42,0x42,0x42,0x42,0x5A,0x5A,0x66,0x42}},
                {'X', {0x42,0x42,0x24,0x18,0x24,0x42,0x42,0x42}},
                {'Y', {0x42,0x42,0x24,0x18,0x08,0x08,0x08,0x08}},
                {'Z', {0x7E,0x02,0x04,0x08,0x10,0x20,0x40,0x7E}},
                {'0', {0x3C,0x42,0x46,0x4A,0x52,0x62,0x42,0x3C}},
                {'1', {0x08,0x18,0x28,0x08,0x08,0x08,0x08,0x3E}},
                {'2', {0x3C,0x42,0x02,0x0C,0x10,0x20,0x40,0x7E}},
                {'3', {0x7E,0x04,0x08,0x0C,0x02,0x42,0x42,0x3C}},
                {'4', {0x04,0x0C,0x14,0x24,0x44,0x7E,0x04,0x04}},
                {'5', {0x7E,0x40,0x7C,0x02,0x02,0x42,0x42,0x3C}},
                {'6', {0x1C,0x20,0x40,0x7C,0x42,0x42,0x42,0x3C}},
                {'7', {0x7E,0x02,0x04,0x08,0x10,0x10,0x10,0x10}},
                {'8', {0x3C,0x42,0x42,0x3C,0x42,0x42,0x42,0x3C}},
                {'9', {0x3C,0x42,0x42,0x3E,0x02,0x02,0x04,0x38}},
                {':', {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}},
                {'-', {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}},
                {'.', {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}},
                {'=', {0x00,0x00,0x7E,0x00,0x00,0x7E,0x00,0x00}},
                {'/', {0x00,0x02,0x04,0x08,0x10,0x20,0x40,0x00}},
            };
            constexpr int FONT_N = (int)(sizeof(FONT) / sizeof(FONT[0]));

            const uint8_t* glyph_of(char c) {
                for (int i = 0; i < FONT_N; i++) if (FONT[i].c == c) return FONT[i].r;
                return FONT[0].r;
            }
        }

        bool gfx_init(uint32_t w, uint32_t h, uint32_t bpp) {
            uint64_t bar0 = find_vga_bar0();
            if (bar0 == 0) {
                Interrupts::klog("gfx: no VGA controller found on PCI");
                return false;
            }
            Interrupts::klog_hex("gfx: VGA BAR0", bar0);

            vbe_write(0, 0xB0C4);
            vbe_write(1, (uint16_t)w);
            vbe_write(2, (uint16_t)h);
            vbe_write(3, (uint16_t)bpp);
            vbe_write(4, 0x41);

            fb.addr = bar0;
            fb.width = w;
            fb.height = h;
            fb.pitch = w * (bpp / 8);
            fb.bpp = bpp;

            uint64_t bytes = (uint64_t)fb.pitch * h;
            uint64_t pages = (bytes + 4095) / 4096;
            for (uint64_t i = 0; i < pages; i++) {
                uint64_t pa = bar0 + i * 4096;
                if (!Memory::vmm_map_page(pa, pa, Memory::VMM_PRESENT | Memory::VMM_WRITABLE)) {
                    Interrupts::klog("gfx: FATAL mapping framebuffer");
                    return false;
                }
            }

            // Backbuffer offscreen para double buffering
            back = (uint32_t*)Memory::kmalloc(bytes);
            if (back == nullptr) {
                Interrupts::klog("gfx: FATAL no backbuffer");
                return false;
            }

            fb.ok = true;
            Interrupts::klog("gfx: framebuffer online 800x600x32 (double buffered)");
            return true;
        }

        void gfx_disable() {
            vbe_write(4, 0x00);
            fb.ok = false;
        }

        const FBInfo& gfx_info() { return fb; }

        void gfx_present() {
            if (!fb.ok || back == nullptr) return;
            uint32_t* dst = (uint32_t*)fb.addr;
            uint32_t n = fb.width * fb.height;
            for (uint32_t i = 0; i < n; i++) dst[i] = back[i];
        }

        void gfx_pixel(uint32_t x, uint32_t y, uint32_t color) {
            if (!fb.ok || back == nullptr || x >= fb.width || y >= fb.height) return;
            back[(uint64_t)y * (fb.pitch / 4) + x] = color;
        }

        void gfx_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
            for (uint32_t j = y; j < y + h; j++)
                for (uint32_t i = x; i < x + w; i++)
                    gfx_pixel(i, j, color);
        }

        void gfx_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
            gfx_fill(x, y, w, 1, color);
            gfx_fill(x, y + h - 1, w, 1, color);
            gfx_fill(x, y, 1, h, color);
            gfx_fill(x + w - 1, y, 1, h, color);
        }

        void gfx_char(uint32_t x, uint32_t y, char c, uint32_t fg) {
            const uint8_t* g = glyph_of(c);
            for (int r = 0; r < 8; r++) {
                uint8_t bits = g[r];
                for (int col = 0; col < 8; col++) {
                    if (bits & (0x80 >> col)) gfx_pixel(x + col, y + r, fg);
                }
            }
        }

        void gfx_text(uint32_t x, uint32_t y, const char* s, uint32_t fg) {
            while (*s) { gfx_char(x, y, *s, fg); x += 8; s++; }
        }
    }
}
