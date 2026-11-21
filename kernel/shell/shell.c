/*
 * Basalt shell (M1) - "bsh", an early-Linux-flavoured framebuffer shell.
 * Commands: help, ver, clear, echo, meminfo, plugins, halt
 * Pure C at the ABI boundary; console/pmm/plugins are reached via sh_ wrappers.
 */
#include "shell.h"
#include "io/kbd.h"
#include <stdint.h>
#include <stddef.h>

/* C-ABI bridges into C++/C subsystems */
void sh_putc(char c);
void sh_write(const char *s);
void sh_clear(void);
void sh_kprintf(const char *fmt, ...);
void sh_halt(void);
uint64_t sh_pmm_total(void);
uint64_t sh_pmm_used(void);
uint64_t sh_plugins_count(void);
const char *sh_plugin_name(uint64_t i);

#define CMD_MAX 128

static void sh_banner(void)
{
    sh_kprintf("\nBasaltCore shell (bsh) v0.1.0\n");
    if (sh_plugins_count() > 0) {
        sh_kprintf("%u plugin(s) installed:", (unsigned)sh_plugins_count());
        for (uint64_t i = 0; i < sh_plugins_count(); i++)
            sh_kprintf(" %s", sh_plugin_name(i));
        sh_kprintf("\n");
    } else {
        sh_kprintf("No plugins installed.\n");
        sh_kprintf("Hint: search 'basalt plugin' on GitHub (github.com/BS-O/Basalt) to find Basalt plugins.\n");
    }
    sh_kprintf("Type 'help' for commands.\n\n");
}

static void sh_help(void)
{
    sh_kprintf("Commands:\n");
    sh_kprintf("  help     - list commands\n");
    sh_kprintf("  ver      - kernel version\n");
    sh_kprintf("  clear    - clear screen\n");
    sh_kprintf("  echo ... - print arguments\n");
    sh_kprintf("  meminfo  - memory statistics\n");
    sh_kprintf("  plugins  - list installed plugins\n");
    sh_kprintf("  halt     - halt the machine\n");
}

static void sh_ver(void)
{
    sh_kprintf("BasaltCore 0.1.0 (M1) - x86-64 kernel, Limine Boot Protocol\n");
    sh_kprintf("Copyright (c) 2026 Basalt Project. C++17/C/asm, from bare metal.\n");
}

static void sh_meminfo(void)
{
    uint64_t total = sh_pmm_total(), used = sh_pmm_used();
    sh_kprintf("Memory:\n");
    sh_kprintf("  frames total : %u (%u KiB)\n", (unsigned)total, (unsigned)(total * 4));
    sh_kprintf("  frames used  : %u (%u KiB)\n", (unsigned)used, (unsigned)(used * 4));
    sh_kprintf("  frames free   : %u (%u KiB)\n", (unsigned)(total - used), (unsigned)((total - used) * 4));
}

static void sh_plugins(void)
{
    uint64_t n = sh_plugins_count();
    if (n == 0) {
        sh_kprintf("No plugins installed.\n");
        sh_kprintf("Hint: search 'basalt plugin' on GitHub (github.com/BS-O/Basalt) to find Basalt plugins.\n");
        return;
    }
    sh_kprintf("%u plugin(s) installed:\n", (unsigned)n);
    for (uint64_t i = 0; i < n; i++)
        sh_kprintf("  [%u] %s\n", (unsigned)i, sh_plugin_name(i));
}

static unsigned long s_strlen(const char *s) { unsigned long n = 0; while (s[n]) n++; return n; }
static int s_streq(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }
static int s_starts(const char *a, const char *p) { while (*p) { if (*a != *p) return 0; a++; p++; } return 1; }

static void exec_cmd(char *cmd)
{
    while (*cmd == ' ') cmd++;
    size_t n = s_strlen(cmd);
    while (n && cmd[n - 1] == ' ') cmd[--n] = 0;
    if (n == 0) return;

    if (s_streq(cmd, "help")) { sh_help(); return; }
    if (s_streq(cmd, "ver")) { sh_ver(); return; }
    if (s_streq(cmd, "clear")) { sh_clear(); return; }
    if (s_streq(cmd, "meminfo")) { sh_meminfo(); return; }
    if (s_streq(cmd, "plugins")) { sh_plugins(); return; }
    if (s_streq(cmd, "halt")) { sh_kprintf("Halting...\n"); sh_halt(); }
    if (s_starts(cmd, "echo ")) { sh_kprintf("%s\n", cmd + 5); return; }
    sh_kprintf("bsh: %s: command not found (try 'help')\n", cmd);
}

void shell_run(void)
{
    kbd_init();
    sh_banner();

    static char cmd[CMD_MAX];
    for (;;) {
        sh_write("basalt# ");
        size_t n = 0;
        for (;;) {
            char c = kbd_getchar();
            if (c == '\n') { sh_putc('\n'); break; }
            if (c == '\b') {
                if (n) { n--; sh_write("\b \b"); }
                continue;
            }
            if (c == '\t') c = ' ';
            if (n < CMD_MAX - 1 && c >= 32) {
                cmd[n++] = c;
                sh_putc(c);
            }
        }
        cmd[n] = 0;
        exec_cmd(cmd);
    }
}
