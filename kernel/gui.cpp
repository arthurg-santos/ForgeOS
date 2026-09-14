#include "gui.h"
#include "graphics.h"
#include "mouse.h"
#include "keyboard.h"
#include "scheduler.h"
#include "timer.h"
#include "pmm.h"
#include "serial.h"
#include "io_ports.h"

namespace Forge {
    namespace Gui {
        namespace {
            bool g_active = false;
            bool g_started = false;
            uint64_t g_task = 0;
            int keylog = 0;

            struct Win {
                int x, y, w, h;
                const char* title;
                uint32_t accent;
                bool visible;
            };
            Win wins[2] = {
                {60, 50, 340, 200, "ABOUT FORGEOS", 0x89B4FA, true},
                {430, 130, 310, 210, "SYSTEM MONITOR", 0xA6E3A1, true},
            };
            constexpr int TITLE_H = 18;

            // Barra de tarefas e menu Iniciar
            constexpr int BAR_H = 22;
            constexpr int BAR_Y = 600 - BAR_H;
            constexpr int START_W = 64;
            constexpr int MENU_W = 170;
            constexpr int ITEM_H = 22;
            constexpr int MENU_ITEMS = 5;
            constexpr int MENU_H = MENU_ITEMS * ITEM_H;
            constexpr int MENU_Y = BAR_Y - MENU_H;
            const char* MENU_LABEL[MENU_ITEMS] = {
                "ABOUT", "MONITOR", "SAIR DA GUI", "REINICIAR", "DESLIGAR"
            };
            bool menu_open = false;
            int want_exit = 0;

            int mouse_x = 400, mouse_y = 300;
            int drag_id = -1, drag_ox = 0, drag_oy = 0;
            uint8_t prev_buttons = 0;

            void str_cat(char* dst, const char* src) {
                while (*dst) dst++;
                while ((*dst = *src)) { dst++; src++; }
            }
            void num_into(char* dst, uint64_t v) {
                char t[24];
                t[20] = '\0';
                int i = 19;
                if (v == 0) t[i--] = '0';
                while (v > 0) { t[i--] = (char)('0' + (v % 10)); v /= 10; }
                str_cat(dst, &t[i + 1]);
            }

            inline void outw16(uint16_t port, uint16_t v) {
                __asm__ __volatile__("outw %0, %1" : : "a"(v), "Nd"(port));
            }
            void reboot_now() {
                Interrupts::klog("gui: reboot requested");
                outb(0x64, 0xFE); // pulso de reset no controlador de teclado
                while (true) __asm__ __volatile__("hlt");
            }
            void shutdown_now() {
                Interrupts::klog("gui: shutdown requested");
                outw16(0x604, 0x2000); // power-off ACPI/Bochs (QEMU)
                while (true) __asm__ __volatile__("hlt");
            }

            const uint16_t CURSOR[14] = {
                0b100000000000, 0b110000000000, 0b111000000000, 0b111100000000,
                0b111110000000, 0b111111000000, 0b111111100000, 0b111111110000,
                0b111111111000, 0b111111111100, 0b111110000000, 0b001100000000,
                0b001100000000, 0b001100000000,
            };

            void draw_cursor(int x, int y) {
                for (int r = 0; r < 14; r++)
                    for (int c = 0; c < 12; c++)
                        if (CURSOR[r] & (0x800 >> c))
                            Graphics::gfx_pixel(x + c + 1, y + r + 1, 0x000000);
                for (int r = 0; r < 14; r++)
                    for (int c = 0; c < 12; c++)
                        if (CURSOR[r] & (0x800 >> c))
                            Graphics::gfx_pixel(x + c, y + r, 0xFFFFFF);
            }

            void draw_window(const Win& w, int id) {
                using namespace Graphics;
                gfx_fill(w.x, w.y, w.w, w.h, 0x1E1E2E);
                gfx_rect(w.x, w.y, w.w, w.h, 0x45475A);
                gfx_fill(w.x + 1, w.y + 1, w.w - 2, TITLE_H, w.accent);
                gfx_text(w.x + 6, w.y + 5, w.title, 0x11111B);
                gfx_fill(w.x + w.w - 16, w.y + 3, 12, 12, 0xF38BA8);
                gfx_char(w.x + w.w - 13, w.y + 5, 'X', 0x11111B);

                if (id == 0) {
                    gfx_text(w.x + 12, w.y + 34, "FORGEOS - SO EM C++ FREESTANDING", 0xCDD6F4);
                    gfx_text(w.x + 12, w.y + 50, "RING 3 + SYSCALLS + VFS + ELF", 0xCDD6F4);
                    gfx_text(w.x + 12, w.y + 66, "ARRASTE AS JANELAS COM O MOUSE", 0xCDD6F4);
                    gfx_text(w.x + 12, w.y + 82, "O QUADRADINHO ROSA FECHA", 0xCDD6F4);
                    gfx_text(w.x + 12, w.y + 98, "MENU INICIAR > SAIR DA GUI", 0xF38BA8);
                } else {
                    char b[64];
                    b[0] = '\0'; str_cat(b, "UPTIME: ");
                    num_into(b, Drivers::timer_ticks() / 100); str_cat(b, " S");
                    gfx_text(w.x + 12, w.y + 34, b, 0xCDD6F4);

                    b[0] = '\0'; str_cat(b, "TICKS: ");
                    num_into(b, Drivers::timer_ticks());
                    gfx_text(w.x + 12, w.y + 50, b, 0xCDD6F4);

                    b[0] = '\0'; str_cat(b, "FREE: ");
                    num_into(b, Memory::pmm_free_pages() * 4); str_cat(b, " KIB");
                    gfx_text(w.x + 12, w.y + 66, b, 0xCDD6F4);

                    b[0] = '\0'; str_cat(b, "PROCS: ");
                    num_into(b, Kernel::proc_count());
                    gfx_text(w.x + 12, w.y + 82, b, 0xCDD6F4);

                    b[0] = '\0'; str_cat(b, "MOUSE: ");
                    num_into(b, (uint64_t)mouse_x); str_cat(b, ",");
                    num_into(b, (uint64_t)mouse_y);
                    gfx_text(w.x + 12, w.y + 98, b, 0xA6E3A1);
                }
            }

            void draw_taskbar() {
                using namespace Graphics;
                gfx_fill(0, BAR_Y, 800, BAR_H, 0x313244);
                gfx_fill(0, BAR_Y, 1, BAR_H, 0x45475A);
                gfx_fill(0, BAR_Y, START_W, BAR_H, menu_open ? 0x45475A : 0x89B4FA);
                gfx_text(6, BAR_Y + 7, "INICIAR", 0x11111B);
                char ub[32];
                ub[0] = '\0'; str_cat(ub, "UPTIME: ");
                num_into(ub, Drivers::timer_ticks() / 100); str_cat(ub, " S");
                gfx_text(690, BAR_Y + 7, ub, 0xCDD6F4);

                if (menu_open) {
                    gfx_fill(0, MENU_Y, MENU_W, MENU_H, 0x1E1E2E);
                    gfx_rect(0, MENU_Y, MENU_W, MENU_H, 0x45475A);
                    for (int i = 0; i < MENU_ITEMS; i++) {
                        int ry = MENU_Y + i * ITEM_H;
                        bool hover = (mouse_x < MENU_W && mouse_y >= ry && mouse_y < ry + ITEM_H);
                        if (hover) gfx_fill(1, ry + 1, MENU_W - 2, ITEM_H - 2, 0x45475A);
                        gfx_text(8, ry + 7, MENU_LABEL[i], 0xCDD6F4);
                    }
                }
            }

            void compose() {
                using namespace Graphics;
                const FBInfo& fbi = gfx_info();
                for (uint32_t y = 0; y < fbi.height; y++) {
                    uint32_t r = (y * 20) / fbi.height;
                    uint32_t g = (y * 40) / fbi.height;
                    uint32_t b = 60 + (y * 90) / fbi.height;
                    gfx_fill(0, y, fbi.width, 1, (r << 16) | (g << 8) | b);
                }
                gfx_text(660, 560, "FORGEOS V0.10", 0xCDD6F4);

                for (int i = 0; i < 2; i++) {
                    if (wins[i].visible) draw_window(wins[i], i);
                }
                draw_taskbar();
                draw_cursor(mouse_x, mouse_y);
                gfx_present(); // flip único por frame: sem flicker
            }

            bool in_rect(int px, int py, int x, int y, int w, int h) {
                return px >= x && px < x + w && py >= y && py < y + h;
            }

            void gui_main() {
                if (!Graphics::gfx_init(800, 600, 32)) {
                    Kernel::task_exit();
                    return;
                }
                Drivers::mouse_init();
                g_task = Kernel::current_task_id();
                g_active = true;

                uint64_t last_refresh = 0;
                bool dirty = true;

                while (true) {
                    int c = Drivers::keyboard_get_char();
                    if (c >= 0) {
                        if (keylog < 5) {
                            Interrupts::klog_hex("gui: key", (uint64_t)c);
                            keylog = keylog + 1;
                        }
                        if (c == 8 || c == 'q') break;
                    }
                    if (want_exit) break;

                    Drivers::MouseEvent ev;
                    while (Drivers::mouse_get_event(&ev)) {
                        mouse_x += ev.dx;
                        mouse_y -= ev.dy;
                        if (mouse_x < 0) mouse_x = 0;
                        if (mouse_x > 799) mouse_x = 799;
                        if (mouse_y < 0) mouse_y = 0;
                        if (mouse_y > 599) mouse_y = 599;

                        uint8_t left = ev.buttons & 1;
                        uint8_t pressed = left && !(prev_buttons & 1);
                        uint8_t released = !left && (prev_buttons & 1);
                        prev_buttons = ev.buttons;
                        bool handled = false;

                        if (pressed) {
                            if (menu_open) {
                                if (in_rect(mouse_x, mouse_y, 0, MENU_Y, MENU_W, MENU_H)) {
                                    int idx = (mouse_y - MENU_Y) / ITEM_H;
                                    menu_open = false;
                                    if (idx == 0) wins[0].visible = !wins[0].visible;
                                    else if (idx == 1) wins[1].visible = !wins[1].visible;
                                    else if (idx == 2) want_exit = 1;
                                    else if (idx == 3) reboot_now();
                                    else if (idx == 4) shutdown_now();
                                } else {
                                    menu_open = false;
                                }
                                handled = true;
                                dirty = true;
                            }
                            if (!handled && in_rect(mouse_x, mouse_y, 0, BAR_Y, START_W, BAR_H)) {
                                menu_open = true;
                                handled = true;
                                dirty = true;
                            }
                            if (!handled) {
                                for (int i = 1; i >= 0; i--) {
                                    Win& w = wins[i];
                                    if (!w.visible) continue;
                                    if (in_rect(mouse_x, mouse_y, w.x + w.w - 16, w.y + 3, 12, 12)) {
                                        w.visible = false;
                                        handled = true;
                                        break;
                                    }
                                    if (in_rect(mouse_x, mouse_y, w.x, w.y, w.w, TITLE_H)) {
                                        drag_id = i;
                                        drag_ox = mouse_x - w.x;
                                        drag_oy = mouse_y - w.y;
                                        handled = true;
                                        break;
                                    }
                                }
                                dirty = true;
                            }
                        } else if (released) {
                            drag_id = -1;
                        } else if (left && drag_id >= 0) {
                            Win& w = wins[drag_id];
                            w.x = mouse_x - drag_ox;
                            w.y = mouse_y - drag_oy;
                            dirty = true;
                        } else {
                            dirty = true;
                        }
                    }

                    uint64_t t = Drivers::timer_ticks();
                    if (t - last_refresh >= 50) { last_refresh = t; dirty = true; }

                    if (dirty) { compose(); dirty = false; }
                    Kernel::yield();
                }

                g_active = false;
                Interrupts::klog("gui: session ended, returning to text mode");
                Graphics::gfx_disable();
                Kernel::task_exit();
            }
        }

        void gui_start() {
            if (g_started) return;
            g_started = true;
            Kernel::task_create(gui_main);
        }

        bool active() { return g_active; }
        uint64_t task_id() { return g_task; }
    }
}
