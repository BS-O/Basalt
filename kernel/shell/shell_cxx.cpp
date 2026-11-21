/*
 * Basalt - shell C-ABI bridges (C++ side).
 * Exposes console/kprintf/pmm/plugins/halt to the pure-C shell as sh_* symbols.
 */
#include "console/console.h"
#include "cpu/halt.h"
#include "lib/kprintf.h"
#include "mm/pmm.h"
#include "plugins/plugins.h"
#include "string.h"
#include <stdint.h>

extern "C" {

void sh_putc(char c) { basalt::console::putc(c); }
void sh_write(const char *s) { basalt::console::write(s); }
void sh_clear(void) { basalt::console::fb_clear(); }
void sh_kprintf(const char *fmt, ...) { va_list ap; va_start(ap, fmt); kvprintf(fmt, ap); va_end(ap); }
void sh_halt(void) { basalt::halt(); }
uint64_t sh_pmm_total(void) { return pmm_total_frames(); }
uint64_t sh_pmm_used(void) { return pmm_used_frames(); }
uint64_t sh_plugins_count(void) { return plugins_count(); }
const char *sh_plugin_name(uint64_t i) { return plugin_name(i); }

} // extern "C"
