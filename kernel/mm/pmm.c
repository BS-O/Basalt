/*
 * Basalt - physical memory manager (M1): bitmap allocator over the Limine memmap.
 *
 * Early-Linux-flavoured and deliberately simple:
 *   - one static bitmap, 1 bit per 4KiB frame, 1 = in use
 *   - usable memmap regions are merged into the bitmap space
 *   - allocation is a linear first-free scan (plenty for M1)
 */
#include "pmm.h"
#include <limine.h>

extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;

#define FRAME_SIZE 4096u
#define MAX_FRAMES (256u * 1024u * 1024u / 4096u) /* tracks up to 1 GiB of frames */

static uint8_t bitmap[MAX_FRAMES / 8];
static uint64_t frame_count;   /* frames managed = highest_usable_paddr >> 12 */
static uint64_t used_count;
static uint64_t usable_kib;    /* total usable memory discovered */
static uint64_t hhdm;
static int inited;

static void mark_frames(uint64_t base, uint64_t len, int used)
{
    uint64_t first = (base + FRAME_SIZE - 1) / FRAME_SIZE;
    uint64_t last = (base + len) / FRAME_SIZE;
    if (last > frame_count) last = frame_count;
    for (uint64_t i = first; i < last; i++) {
        int bit = (int)(i & 7);
        int byte = (int)(i >> 3);
        if (used) {
            if (!(bitmap[byte] & (1u << bit))) { bitmap[byte] |= (uint8_t)(1u << bit); used_count++; }
        } else {
            if (bitmap[byte] & (1u << bit)) { bitmap[byte] &= (uint8_t)~(1u << bit); used_count--; }
        }
    }
}

uint64_t pmm_init(void)
{
    if (inited) return usable_kib;
    if (memmap_request.response == 0) return 0;
    if (hhdm_request.response == 0) return 0;

    hhdm = hhdm_request.response->offset;
    struct limine_memmap_response *mm = memmap_request.response;

    uint64_t max_paddr = 0;
    for (uint64_t i = 0; i < mm->entry_count; i++) {
        struct limine_memmap_entry *e = mm->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE && e->base + e->length > max_paddr)
            max_paddr = e->base + e->length;
    }
    if (max_paddr == 0) return 0;
    frame_count = (max_paddr + FRAME_SIZE - 1) / FRAME_SIZE;
    if (frame_count > MAX_FRAMES) frame_count = MAX_FRAMES;

    /* mark everything in use, then free usable ranges */
    for (uint64_t i = 0; i < frame_count; i++) {
        bitmap[i >> 3] |= (uint8_t)(1u << (i & 7));
    }
    used_count = frame_count;

    for (uint64_t i = 0; i < mm->entry_count; i++) {
        struct limine_memmap_entry *e = mm->entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE) continue;
        uint64_t first = (e->base + FRAME_SIZE - 1) / FRAME_SIZE;
        uint64_t last = (e->base + e->length) / FRAME_SIZE;
        if (last > frame_count) last = frame_count;
        for (uint64_t f = first; f < last; f++)
            bitmap[f >> 3] &= (uint8_t)~(1u << (f & 7));
        used_count -= (last > first ? last - first : 0);
        usable_kib += e->length / 1024;
    }
    inited = 1;
    return usable_kib;
}

basalt_paddr pmm_alloc_frame(void)
{
    if (!inited) return 0;
    for (uint64_t i = 0; i < frame_count; i++) {
        if (!(bitmap[i >> 3] & (1u << (i & 7)))) {
            bitmap[i >> 3] |= (uint8_t)(1u << (i & 7));
            used_count++;
            return i * FRAME_SIZE;
        }
    }
    return 0;
}

void pmm_free_frame(basalt_paddr frame)
{
    if (!inited || frame == 0) return;
    uint64_t i = frame / FRAME_SIZE;
    if (i >= frame_count) return;
    bitmap[i >> 3] &= (uint8_t)~(1u << (i & 7));
    used_count--;
}

uint64_t pmm_total_frames(void) { return frame_count; }
uint64_t pmm_used_frames(void)  { return used_count; }
uint64_t pmm_hhdm_offset(void)  { return hhdm; }
