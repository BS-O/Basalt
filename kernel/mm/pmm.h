/* Basalt - physical memory manager interface (M0 PLACEHOLDER, no implementation).
 *
 * M0 scope is "boot + hello world" only. The real PMM (first milestone M1)
 * will consume the Limine memmap response (LIMINE_MEMMAP_REQUEST_ID) and the
 * HHDM offset. Interface frozen now so subsystems can compile against it.
 */
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t basalt_paddr;

/* Returns 0 on success; M0 stub always returns 0 without doing anything. */
int pmm_init(void);

/* Allocate one 4KiB physical frame; returns 0 when out of memory.
 * M0 stub always returns 0. */
basalt_paddr pmm_alloc_frame(void);

void pmm_free_frame(basalt_paddr frame);

#ifdef __cplusplus
}
#endif
