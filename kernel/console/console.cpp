/*
 * Basalt - console output abstraction implementation (L3).
 * Fans characters out to every configured sink.
 */
#include "console.h"

extern "C" {
void serial_init(void);
void serial_putc(char c);
void fb_init(void *addr, uint64_t width, uint64_t height, uint64_t pitch,
             uint16_t bpp, uint8_t memory_model);
void fb_putc(char c);
void fb_clear(void);
int fb_ok(void);
}

namespace basalt::console {

void fb_clear(void) { ::fb_clear(); }
int fb_available(void) { return ::fb_ok(); }

void init(const fb_info *info)
{
    serial_init(); /* always: debug/CI channel */

    if (info != nullptr && info->address != nullptr) {
        fb_init(info->address, info->width, info->height, info->pitch,
                info->bpp, info->memory_model);
        if (!fb_ok())
            serial_putc('!'); /* framebuffer init failed, serial-only fallback */
    }
    /* info == nullptr: serial-only bootstrap pass, no marker needed */
}

void putc(char c)
{
    if (c == '\n') {
        fb_putc('\n');
        serial_putc('\r');
        serial_putc('\n');
        return;
    }
    fb_putc(c);
    serial_putc(c);
}

void write(const char *s)
{
    while (*s != '\0')
        putc(*s++);
}

} // namespace basalt::console
