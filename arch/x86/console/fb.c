/*
 * Basalt - framebuffer text console (L1 HAL).
 *
 * Draws 8x16 glyphs from the generated Terminus font (third_party/terminus,
 * ADR-004) into a 32bpp linear RGB framebuffer provided by Limine (ADR-002).
 * Includes newline handling with simple scroll.
 */
#include "fb.h"
#include "terminus_font.h"

#define FG_WHITE 0x00FFFFFFu
#define GLYPH_H FONT_H
#define GLYPH_W FONT_W

static volatile uint32_t *fb_base;
static uint64_t fb_width, fb_height, fb_pitch;
static int fb_active = 0;
static uint64_t cur_x, cur_y; /* in pixels */

void fb_init(void *addr, uint64_t width, uint64_t height, uint64_t pitch,
             uint16_t bpp, uint8_t memory_model)
{
    if (addr == 0 || bpp != 32 || memory_model != 1 /* LIMINE_FRAMEBUFFER_RGB */) {
        fb_active = 0;
        return;
    }
    fb_base = (volatile uint32_t *)addr;
    fb_width = width;
    fb_height = height;
    fb_pitch = pitch;
    cur_x = 0;
    cur_y = 0;
    fb_active = 1;

    /* clear screen to black */
    for (uint64_t row = 0; row < fb_height; row++) {
        volatile uint32_t *line =
            (volatile uint32_t *)((volatile uint8_t *)fb_base + row * fb_pitch);
        for (uint64_t col = 0; col < fb_width; col++)
            line[col] = 0;
    }
}

int fb_ok(void)
{
    return fb_active;
}

static void fb_newline(void)
{
    cur_x = 0;
    cur_y += GLYPH_H;
    if (cur_y + GLYPH_H > fb_height) {
        /* scroll one text row up: memmove, then clear the last row */
        uint64_t move_bytes = (fb_height - GLYPH_H) * fb_pitch;
        volatile uint8_t *base = (volatile uint8_t *)fb_base;
        for (uint64_t i = 0; i < move_bytes; i++)
            base[i] = base[i + GLYPH_H * fb_pitch];
        for (uint64_t row = fb_height - GLYPH_H; row < fb_height; row++) {
            volatile uint32_t *line =
                (volatile uint32_t *)(base + row * fb_pitch);
            for (uint64_t col = 0; col < fb_width; col++)
                line[col] = 0;
        }
        cur_y = fb_height - GLYPH_H;
    }
}

void fb_putc(char c)
{
    if (!fb_active)
        return;

    if (c == '\n') {
        fb_newline();
        return;
    }
    if (c == '\r') {
        cur_x = 0;
        return;
    }

    if (c < FONT_FIRST || c > FONT_LAST)
        c = '?';

    const uint8_t *glyph = &font_glyphs[(c - FONT_FIRST) * GLYPH_H];
    for (int row = 0; row < GLYPH_H; row++) {
        uint8_t bits = glyph[row];
        volatile uint32_t *line =
            (volatile uint32_t *)((volatile uint8_t *)fb_base +
                                  (cur_y + row) * fb_pitch);
        for (int col = 0; col < GLYPH_W; col++) {
            if (bits & (0x80u >> col))
                line[cur_x + col] = FG_WHITE;
        }
    }

    cur_x += GLYPH_W;
    if (cur_x + GLYPH_W > fb_width)
        fb_newline();
}
