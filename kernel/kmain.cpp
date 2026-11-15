/*
 * Basalt - kernel entry point (L3, C++).
 *
 * Called from boot/x86/entry64.S once Limine has done all firmware work
 * (ADR-002): we are in x86-64 long mode, on our own stack, and the
 * framebuffer request has been filled in (or not - handled gracefully).
 */
#include <limine.h>

#include "console/console.h"
#include "cpu/halt.h"

extern "C" void dbg_mark(char c);

extern "C" void kmain(struct limine_framebuffer_request *fb_request)
{
    dbg_mark('K'); /* reached kmain */

    basalt::console::fb_info info{};

    const volatile struct limine_framebuffer_request *req = fb_request;
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
    } else {
        basalt::console::init(nullptr);
    }

    basalt::console::write("hello world\n");

    basalt::halt();
}
