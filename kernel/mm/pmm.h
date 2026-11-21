/* Basalt - physical memory manager interface (M1). */
#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t basalt_paddr;

/* Initialize from the Limine memmap response. Returns total usable KiB, 0 on failure. */
uint64_t pmm_init(void);

/* Allocate one 4KiB physical frame; returns 0 when out of memory. */
basalt_paddr pmm_alloc_frame(void);

/* Free one 4KiB physical frame previously returned by pmm_alloc_frame. */
void pmm_free_frame(basalt_paddr frame);

/* Allocator stats for the boot log / shell. */
uint64_t pmm_total_frames(void);
uint64_t pmm_used_frames(void);
uint64_t pmm_hhdm_offset(void);

#ifdef __cplusplus
}
#endif
