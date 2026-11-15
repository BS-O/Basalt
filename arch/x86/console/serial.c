/*
 * Basalt - 16550 UART driver, COM1 0x3F8, 115200 8N1, polling (L1 HAL).
 * Deliberately minimal: debug logging + CI assertion channel (ADR-003).
 */
#include "serial.h"
#include "io/io.h"

#define COM1 0x3F8

#define COM_DATA (COM1 + 0)
#define COM_IER (COM1 + 1)
#define COM_FCR (COM1 + 2)
#define COM_LCR (COM1 + 3)
#define COM_MCR (COM1 + 4)
#define COM_LSR (COM1 + 5)

#define LSR_THRE 0x20

static int serial_ready = 0;

void serial_init(void)
{
    outb(COM_IER, 0x00); /* disable UART interrupts (polling only) */
    outb(COM_LCR, 0x80); /* DLAB on */
    outb(COM_DATA, 0x01); /* divisor low: 1 -> 115200 baud */
    outb(COM_IER, 0x00); /* divisor high */
    outb(COM_LCR, 0x03); /* 8N1, DLAB off */
    outb(COM_FCR, 0xC7); /* enable FIFO, clear, 14-byte trigger */
    outb(COM_MCR, 0x0B); /* DTR, RTS, OUT2 */
    serial_ready = 1;
}

void serial_putc(char c)
{
    if (!serial_ready)
        return;
    for (unsigned spin = 0; spin < 100000u; spin++) {
        if (inb(COM_LSR) & LSR_THRE)
            break;
    }
    outb(COM_DATA, (uint8_t)c);
}
