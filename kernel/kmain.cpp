/*
 * Basalt - kernel entry point (L3, C++).
 *
 * M1: full bring-up with an early-Linux-flavoured boot log:
 *   banner -> memmap -> PMM -> framebuffer -> serial -> plugins -> shell
 */
#include <limine.h>

#include "console/console.h"
#include "cpu/halt.h"
#include "lib/kprintf.h"
#include "mm/pmm.h"
#include "plugins/plugins.h"
#include "shell/shell.h"
#include "string.h"

extern "C" void dbg_mark(char c);

extern "C" void kmain(struct limine_framebuffer_request *fb_request)
{
    dbg_mark('K'); /* reached kmain */

    /* ---- console: serial first (so the log can start), framebuffer next ---- */
    basalt::console::init(nullptr); /* serial-only boot strap */

    kprintf("BasaltCore 0.1.0 (x86-64) - Limine Boot Protocol, base revision 3\n");
    kprintf("Copyright (c) 2026 Basalt Project. booting...\n\n");

    /* ---- framebuffer ---- */
    basalt::console::fb_info info{};
    const volatile struct limine_framebuffer_request *req = fb_request;
    int fb_up = 0;
    if (req != nullptr && req->response != nullptr &&
        req->response->framebuffer_count > 0) {
        volatile struct limine_framebuffer *fb = req->response->framebuffers[0];
        info.address = fb->address;
        info.width = fb->width;
        info.height = fb->height;
        info.pitch = fb->pitch;
        info.bpp = fb->bpp;
        info.memory_model = fb->memory_model;
        basalt::console::init(&info);
        fb_up = basalt::console::fb_available();
    }
    if (fb_up) {
        kprintf("fb0      : %ux%u x %u bpp, pitch %u - framebuffer console up\n",
                (unsigned)info.width, (unsigned)info.height,
                (unsigned)info.bpp, (unsigned)info.pitch);
    } else {
        kprintf("fb0      : unavailable, continuing on COM1\n");
    }

    /* ---- serial ---- */
    kprintf("ttyS0    : COM1 16550, 115200 8N1 - debug/CI console\n");

    /* ---- memory ---- */
    uint64_t usable = pmm_init();
    if (usable) {
        kprintf("mem     : usable %u KiB, %u frames managed by bitmap PMM (hhdm +0x%lx)\n",
                (unsigned)(usable), (unsigned)pmm_total_frames(),
                (unsigned long long)pmm_hhdm_offset());
    } else {
        kprintf("mem     : WARNING - no usable memory map found, PMM offline\n");
    }

    /* ---- keyboard ---- */
    kprintf("i8042    : PS/2 keyboard ok (polled)\n");

    /* ---- plugins ---- */
    uint64_t np = plugins_init();
    if (np > 0) {
        kprintf("plugins  : %u plugin(s) detected:", (unsigned)np);
        for (uint64_t i = 0; i < np; i++)
            kprintf(" %s", plugin_name(i));
        kprintf("\n");
        uint64_t ok = plugins_boot();
        kprintf("plugins  : %u/%u initialized ok\n", (unsigned)ok, (unsigned)np);
    } else {
        kprintf("plugins  : none installed\n");
        kprintf("plugins  : hint - search 'basalt plugin' on GitHub (github.com/BS-O/Basalt)\n");
    }

    kprintf("\nBasaltCore is ready. starting shell...\n\n");

    /* ---- shell (never returns) ---- */
    shell_run();

    basalt::halt();
}
