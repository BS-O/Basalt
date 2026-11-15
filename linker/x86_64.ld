/*
 * Basalt x86_64 kernel linker script (M0).
 *
 * Limine Boot Protocol requirement (learned the hard way, see screendump
 * diagnosis): kernel PHDRs must be in the higher half. The kernel is linked
 * at 0xffffffff80000000; Limine places it physically and maps it there.
 * Physical placement is discovered at runtime via the executable-address
 * request when needed (M1+, not used in M0).
 */

OUTPUT_FORMAT(elf64-x86-64)
ENTRY(boot_entry)

KERNEL_VMA = 0xffffffff80000000;

PHDRS
{
    text   PT_LOAD FLAGS(5); /* R + X */
    rodata PT_LOAD FLAGS(4); /* R     */
    data   PT_LOAD FLAGS(6); /* W + R */
}

SECTIONS
{
    . = KERNEL_VMA;

    .text : ALIGN(0x1000) {
        KEEP(*(.text.boot))
        *(.text .text.*)
    } :text

    /* Limine boot protocol request markers (page-aligned: must not share a
     * 4K page with the executable .text segment; page alignment implies the
     * required 8-byte alignment) */
    .limine_requests : ALIGN(0x1000) {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :rodata

    .rodata : ALIGN(0x1000) {
        *(.rodata .rodata.*)
    } :rodata

    .data : ALIGN(0x1000) {
        *(.data .data.*)
    } :data

    .bss : ALIGN(0x1000) {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /DISCARD/ : {
        *(.comment)
        *(.note .note.*)
        *(.eh_frame .eh_frame.*)
    }
}
