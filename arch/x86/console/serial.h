/* Basalt - COM1 debug serial (L1 HAL). Debug/CI channel only (ADR-003). */
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void serial_init(void);
void serial_putc(char c);

#ifdef __cplusplus
}
#endif
