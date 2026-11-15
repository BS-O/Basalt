/* Basalt - minimal freestanding string/memory routines (lib/kstd). */
#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memset(void *dst, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
size_t strlen(const char *s);

#ifdef __cplusplus
}
#endif
