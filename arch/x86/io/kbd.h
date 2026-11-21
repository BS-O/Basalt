/* Basalt - PS/2 keyboard driver (M1): polled scancode set 1 -> ASCII. */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the PS/2 controller (flush, enable port 1). */
void kbd_init(void);

/* Poll the keyboard; returns the next ASCII key, 0 if none.
 * Handles shift for letters/digits/symbols; Enter=10, Backspace=8, Tab=9. */
char kbd_getchar(void);

#ifdef __cplusplus
}
#endif
