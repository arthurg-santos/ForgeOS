#include "user_syscalls.h"

namespace {
    constexpr int LINE_MAX = 128;
    constexpr int HIST_MAX = 16;

    char line[LINE_MAX];
    int len = 0;          // comprimento atual
    int cur = 0;          // posição do cursor dentro da linha

    char hist[HIST_MAX][LINE_MAX];
    int hist_count = 0;
    int hist_view = -1;   // índice sendo navegado (-1 = linha nova)
    char draft[LINE_MAX]; // rascunho da linha nova durante a navegação

    bool streq(const char* a, const char* b) {
        while (*a && *a == *b) { a++; b++; }
        return *a == *b;
    }

    bool starts_with(const char* s, const char* p) {
        while (*p) { if (*s != *p) return false; s++; p++; }
        return true;
    }

    void cat(char* dst, const char* src) {
        while (*dst) dst++;
        while ((*dst = *src)) { dst++; src++; }
    }

    void copy_str(char* dst, const char* src) {
        int i = 0;
        while (src[i] && i < LINE_MAX - 1) { dst[i] = src[i]; i++; }
        dst[i] = '\0';
    }

    void num_into(char* dst, uint64_t v) {
        char t[24];
        t[20] = '\0';
        int i = 19;
        if (v == 0) t[i--] = '0';
        while (v > 0) { t[i--] = (char)('0' + (v % 10)); v /= 10; }
        cat(dst, &t[i + 1]);
    }

    void write_padded(const char* s, int width) {
        sys_write(s);
        int n = 0;
        while (s[n]) n++;
        for (int i = n; i < width; i++) sys_write(" ");
    }

    void move_left(int n)  { for (int i = 0; i < n; i++) con_left(); }
    void move_right(int n) { for (int i = 0; i < n; i++) con_right(); }

    // ---------- editor de linha ----------

    void ed_insert(char c) {
        if (len >= LINE_MAX - 1) return;
        for (int i = len; i > cur; i--) line[i] = line[i - 1];
        line[cur] = c;
        len++;
        char tmp[LINE_MAX];
        int t = 0;
        for (int i = cur; i < len; i++) tmp[t++] = line[i];
        tmp[t] = '\0';
        sys_write(tmp);                 // imprime o char + a cauda
        move_left(len - (cur + 1));     // reposiciona o cursor
        cur++;
    }

    void ed_backspace() {
        if (cur <= 0) return;
        move_left(1);
        cur--;
        for (int i = cur; i < len - 1; i++) line[i] = line[i + 1];
        len--;
        char tmp[LINE_MAX];
        int t = 0;
        for (int i = cur; i < len; i++) tmp[t++] = line[i];
        tmp[t] = '\0';
        sys_write(tmp);
        sys_write(" ");                 // apaga a sobra no fim
        move_left(len - cur + 1);       // cursor de volta ao ponto de edição
    }

    void ed_delete() {
        if (cur >= len) return;
        for (int i = cur; i < len - 1; i++) line[i] = line[i + 1];
        len--;
        char tmp[LINE_MAX];
        int t = 0;
        for (int i = cur; i < len; i++) tmp[t++] = line[i];
        tmp[t] = '\0';
        sys_write(tmp);
        sys_write(" ");
        move_left(len - cur + 1);
    }

    void ed_left()  { if (cur > 0)  { move_left(1);  cur--; } }
    void ed_right() { if (cur < len) { move_right(1); cur++; } }
    void ed_home()  { move_left(cur); cur = 0; }
    void ed_end()   { move_right(len - cur); cur = len; }

    // Substitui a linha inteira na tela (usado pelo histórico)
    void set_line_from(const char* s) {
        int oldlen = len;
        move_left(cur);
        int n = 0;
        while (s[n] && n < LINE_MAX - 1) { line[n] = s[n]; n++; }
        line[n] = '\0';
        len = n;
        cur = n;
        sys_write(line);
        for (int i = n; i < oldlen; i++) sys_write(" "); // apaga sobra
        move_left(oldlen - n);                           // cursor no fim novo
    }

    void ed_up() {
        if (hist_count == 0) return;
        if (hist_view < 0) { copy_str(draft, line); hist_view = hist_count - 1; }
        else if (hist_view > 0) hist_view--;
        set_line_from(hist[hist_view]);
    }

    void ed_down() {
        if (hist_view < 0) return;
        hist_view++;
        if (hist_view >= hist_count) { hist_view = -1; set_line_from(draft); }
        else set_line_from(hist[hist_view]);
    }

    int read_line() {
        len = 0; cur = 0; hist_view = -1;
        while (true) {
            int c = sys_getchar();
            if (c < 0) { sys_yield(); continue; }
            if (c == '\n') { sys_write("\n"); break; }
            switch (c) {
                case '\b':        ed_backspace(); continue;
                case KEY_UP:      ed_up();        continue;
                case KEY_DOWN:    ed_down();      continue;
                case KEY_LEFT:    ed_left();      continue;
                case KEY_RIGHT:   ed_right();     continue;
                case KEY_HOME:    ed_home();      continue;
                case KEY_END:     ed_end();       continue;
                case KEY_DELETE:  ed_delete();    continue;
                default:
                    if (c >= 32 && c < 127) ed_insert((char)c);
                    continue;
            }
        }
        // Termina a string ANTES de salvar/despachar: sem isso, sobras do
        // comando anterior permanecem invisíveis no buffer (bug "clearfetch").
        line[len] = '\0';
        if (len > 0) {
            if (hist_count == HIST_MAX) {
                for (int i = 0; i < HIST_MAX - 1; i++) copy_str(hist[i], hist[i + 1]);
                hist_count--;
            }
            copy_str(hist[hist_count], line);
            hist_count++;
        }
        return len;
    }

    // ---------- comandos ----------

    void cmd_help() {
        sys_write("fsh commands:\n");
        sys_write("  help        esta ajuda\n");
        sys_write("  echo <txt>  imprime texto\n");
        sys_write("  ps          lista processos\n");
        sys_write("  mem         memoria fisica e heap\n");
        sys_write("  uptime      tempo de atividade\n");
        sys_write("  clear       limpa a tela\n");
        sys_write("  forgefetch  sistema em estilo fastfetch\n");
        sys_write("  exit        encerra o shell\n");
        sys_write("edicao: setas cima/baixo = historico; esq/dir, home, end,\n");
        sys_write("        backspace e delete editam a linha atual\n");
    }

    void cmd_mem() {
        ForgeSysInfo si;
        sys_sysinfo(&si);
        char b[24];
        sys_write("  total: "); put_dec(si.total_kb, b); sys_write(" KiB\n");
        sys_write("  free:  "); put_dec(si.free_kb, b);  sys_write(" KiB\n");
        sys_write("  used:  "); put_dec(si.total_kb - si.free_kb, b); sys_write(" KiB\n");
        sys_write("  heap:  "); put_dec(si.heap_used, b); sys_write(" / ");
        put_dec(si.heap_cap, b); sys_write(" bytes\n");
    }

    void cmd_uptime() {
        ForgeSysInfo si;
        sys_sysinfo(&si);
        char b[24];
        sys_write("  up "); put_dec(si.ticks / 100, b); sys_write(" s (");
        put_dec(si.ticks, b); sys_write(" ticks @ 100Hz)\n");
    }

    void cmd_ps() {
        char buf[64];
        for (uint64_t i = 0; ; i++) {
            if (sys_ps(i, buf) != 0) break;
            sys_write("  ");
            sys_write(buf);
            sys_write("\n");
        }
    }

    const char* LOGO[6] = {
        "  _____",
        " |  ___|__  _ __ __ _  ___",
        " | |_ / _ \\| '__/ _` |/ _ \\",
        " |  _| (_) | | | (_| |  __/",
        " |_|  \\___/|_|  \\__, |\\___|",
        "                |___/",
    };

    void cmd_forgefetch() {
        ForgeSysInfo si;
        sys_sysinfo(&si);

        static char l0[64], l2[96], l4[64], l7[96], l8[96];
        l0[0] = l2[0] = l4[0] = l7[0] = l8[0] = '\0';

        cat(l0, "user@forgeos");
        cat(l2, "OS: ForgeOS v0.8 (x86_64, ring 3)");
        cat(l4, "Uptime: ");
        num_into(l4, si.ticks / 100);
        cat(l4, "s");
        cat(l7, "Memory: ");
        num_into(l7, si.total_kb - si.free_kb);
        cat(l7, " / ");
        num_into(l7, si.total_kb);
        cat(l7, " KiB");
        cat(l8, "Procs: ");
        num_into(l8, si.tasks);
        cat(l8, " | TTY: 80x25 VGA text");

        const char* info[10] = {
            l0,
            "------------------------------------",
            l2,
            "Kernel: forge-0.8 (phase 8: tty+fsh)",
            l4,
            "Shell: fsh 1.0",
            "CPU: x86_64 Long Mode @ PIT 100Hz",
            l7,
            l8,
            ""
        };

        for (int row = 0; row < 10; row++) {
            sys_setcolor(COL_LBROWN, COL_BLACK);
            write_padded(row < 6 ? LOGO[row] : "", 30);
            if (row == 0) sys_setcolor(COL_LGREEN, COL_BLACK);
            else if (row == 1) sys_setcolor(COL_LBROWN, COL_BLACK);
            else sys_setcolor(COL_WHITE, COL_BLACK);
            sys_write(info[row]);
            sys_write("\n");
        }

        sys_setcolor(COL_LBROWN, COL_BLACK);
        write_padded("", 30);
        for (uint64_t c = 1; c <= 15; c++) {
            sys_setcolor(c, c);
            sys_write("  ");
        }
        sys_setcolor(COL_LGREY, COL_BLACK);
        sys_write("\n");
    }

    void dispatch(char* cmd) {
        if (cmd[0] == '\0') return;
        if (streq(cmd, "help"))         { cmd_help(); return; }
        if (streq(cmd, "clear"))        { sys_clear(); return; }
        if (streq(cmd, "ps"))           { cmd_ps(); return; }
        if (streq(cmd, "mem"))          { cmd_mem(); return; }
        if (streq(cmd, "uptime"))       { cmd_uptime(); return; }
        if (streq(cmd, "forgefetch"))   { cmd_forgefetch(); return; }
        if (streq(cmd, "exit"))         { sys_exit(); return; }
        if (starts_with(cmd, "echo "))  { sys_write(cmd + 5); sys_write("\n"); return; }
        sys_write("fsh: command not found: ");
        sys_write(cmd);
        sys_write("\n");
    }
}

extern "C" void _start() {
    sys_setcolor(COL_LGREY, COL_BLACK);
    sys_write("ForgeOS shell (fsh 1.0) - digite 'help' ou 'forgefetch'\n");

    while (true) {
        sys_setcolor(COL_LGREEN, COL_BLACK);
        sys_write("forgeos");
        sys_setcolor(COL_LGREY, COL_BLACK);
        sys_write(":~$ ");
        sys_setcolor(COL_WHITE, COL_BLACK);
        read_line();
        dispatch(line);
    }
}
