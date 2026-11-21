/* Basalt - linear framebuffer text console sink (L1 HAL). */
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Configure the sink; safe to call with addr==0 (sink stays disabled). */
void fb_init(void *addr, uint64_t width, uint64_t height, uint64_t pitch,
             uint16_t bpp, uint8_t memory_model);
void fb_putc(char c);
void fb_clear(void); /* wipe to black, cursor home */
int fb_ok(void); /* nonzero if a usable 32bpp RGB framebuffer is active */

#ifdef __cplusplus
}
#endif
