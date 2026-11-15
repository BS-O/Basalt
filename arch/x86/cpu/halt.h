/* Basalt - park the CPU forever (M0 terminal state). */
#pragma once

#ifdef __cplusplus
namespace basalt {
inline void halt()
{
    for (;;)
        __asm__ volatile("cli; hlt");
}
} // namespace basalt
#else
static inline void halt_forever(void)
{
    for (;;)
        __asm__ volatile("cli; hlt");
}
#endif
