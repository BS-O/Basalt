/*
 * Basalt - PS/2 keyboard driver (M1).
 * Polled scancode set 1 translation, enough for the framebuffer shell.
 * 0x60 data / 0x64 status, i8042 semantics.
 */
#include "kbd.h"
#include "io.h"

#define KBD_DATA 0x60
#define KBD_STATUS 0x64

/* set-1 make codes -> US layout, unshifted */
static const char sc_un[128] = {
    0,   27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, '\\','z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
};
/* shifted */
static const char sc_sh[128] = {
    0,   27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0, 'A','S','D','F','G','H','J','K','L',':','"','~',
    0, '|','Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' ',
};

static int shift_down;
static int caps_down;

void kbd_init(void)
{
    /* drain any pending byte from the controller */
    while (inb(KBD_STATUS) & 1) (void)inb(KBD_DATA);
    shift_down = 0;
    caps_down = 0;
}

char kbd_getchar(void)
{
    for (;;) {
        if (inb(KBD_STATUS) & 1) {
            uint8_t sc = inb(KBD_DATA);
            if (sc & 0x80) {
                uint8_t brk = sc & 0x7f;
                if (brk == 0x2a || brk == 0x36) shift_down = 0;
                if (brk == 0x3a) caps_down = 0;
                continue;
            }
            if (sc == 0x2a || sc == 0x36) { shift_down = 1; continue; }
            if (sc == 0x3a) { caps_down = !caps_down; continue; }
            if (sc >= 128) continue;
            char c = shift_down ? sc_sh[sc] : sc_un[sc];
            if (caps_down && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
            if (c != 0) return c;
        }
        /* no interrupts are enabled in M1 - spin briefly instead of hlt */
        __asm__ volatile("pause");
    }
}
