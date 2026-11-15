/*
 * Basalt - console output abstraction (L3, architecture independent).
 * Multiple sinks are assembled at init(): framebuffer (main display) and
 * COM1 serial (debug/CI, ADR-003). arm64 will provide its own sinks.
 */
#pragma once
#include <stdint.h>

namespace basalt::console {

/* Description of the architecture-provided framebuffer, or nullptr to run
 * serial-only. Kept limine-free on purpose: the console layer must not know
 * about the boot protocol. */
struct fb_info {
    void *address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint16_t bpp;
    uint8_t memory_model;
};

void init(const fb_info *info);
void putc(char c);
void write(const char *s);

} // namespace basalt::console
