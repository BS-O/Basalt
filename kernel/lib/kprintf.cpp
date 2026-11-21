/*
 * Basalt - kprintf (M1): minimal printf subset routed to basalt::console.
 * Deliberately tiny: %s %c %d %u %x (also %lld/%llu/%llx via (long long)).
 */
#include "kprintf.h"
#include <stdarg.h>
#include <stddef.h>
#include "console/console.h"

static void emit(const char *fmt, va_list ap)
{
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') { basalt::console::putc(*p); continue; }
        p++;
        if (*p == 0) break;
        if (*p == 'l') { p++; /* eat one 'l'; we always read long long */ }
        switch (*p) {
        case '%': basalt::console::putc('%'); break;
        case 'c': basalt::console::putc((char)va_arg(ap, int)); break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (s == nullptr) s = "(null)";
            while (*s) basalt::console::putc(*s++);
            break;
        }
        case 'd': {
            long long v = va_arg(ap, long long);
            unsigned long long u = (v < 0) ? (unsigned long long)(-(v + 1)) + 1 : (unsigned long long)v;
            char buf[24]; int i = 0;
            if (v < 0) basalt::console::putc('-');
            if (u == 0) buf[i++] = '0';
            while (u) { buf[i++] = (char)('0' + (u % 10)); u /= 10; }
            while (i) basalt::console::putc(buf[--i]);
            break;
        }
        case 'u': {
            unsigned long long u = va_arg(ap, unsigned long long);
            char buf[24]; int i = 0;
            if (u == 0) buf[i++] = '0';
            while (u) { buf[i++] = (char)('0' + (u % 10)); u /= 10; }
            while (i) basalt::console::putc(buf[--i]);
            break;
        }
        case 'x': {
            unsigned long long u = va_arg(ap, unsigned long long);
            char buf[20]; int i = 0;
            if (u == 0) buf[i++] = '0';
            while (u) { unsigned d = (unsigned)(u & 0xf); buf[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10); u >>= 4; }
            while (i) basalt::console::putc(buf[--i]);
            break;
        }
        default: basalt::console::putc('%'); basalt::console::putc(*p); break;
        }
    }
}

extern "C" void kvprintf(const char *fmt, va_list ap)
{
    emit(fmt, ap);
}

extern "C" void kprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    emit(fmt, ap);
    va_end(ap);
}
