#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "types.h"

namespace Forge {
    namespace Graphics {
        struct FBInfo {
            uint64_t addr;
            uint32_t width, height, pitch, bpp;
            bool ok;
        };

        bool gfx_init(uint32_t w, uint32_t h, uint32_t bpp);
        void gfx_disable();
        const FBInfo& gfx_info();

        // Copia o backbuffer offscreen para o framebuffer de uma vez
        // (double buffering: elimina o flicker de redraw direto).
        void gfx_present();

        void gfx_pixel(uint32_t x, uint32_t y, uint32_t color);
        void gfx_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
        void gfx_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
        void gfx_char(uint32_t x, uint32_t y, char c, uint32_t fg);
        void gfx_text(uint32_t x, uint32_t y, const char* s, uint32_t fg);
    }
}

#endif
