/* Basalt - simple boot-time string formatting (M1): printf subset for the boot log. */
#pragma once
#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* kprintf: format into console. Supports %s %c %d %u %x %%. */
void kprintf(const char *fmt, ...);

/* va_list variant for C-ABI bridges. */
void kvprintf(const char *fmt, va_list ap);

#ifdef __cplusplus
}
#endif
