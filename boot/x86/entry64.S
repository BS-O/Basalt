/*
 * Basalt - 64-bit entry stub (L0, pure assembly).
 *
 * Entered by Limine in x86-64 long mode with interrupts disabled and the
 * kernel mapped (see ADR-002). We only set up our own stack and hand over
 * to the C++ kernel entry point.
 *
 * Serial markers ('1','2','3') are written raw to COM1 for boot debugging;
 * they are harmless and stay for M0 diagnostics.
 */

.section .text.boot, "ax", @progbits
.global boot_entry
boot_entry:
    cli

    /* debug marker: raw COM1 write */
    movq    $0x3f8, %rdx
1:  movl    $0x3fd, %edx
    inb     %dx, %al
    testb   $0x20, %al
    jz      1b
    movl    $0x3f8, %edx
    movb    $'1', %al
    outb    %al, %dx

    /* switch to our own stack (.bss, 16 KiB, 16-byte aligned) */
    movabsq $boot_stack_top, %rsp
    xorq  %rbp, %rbp

    /* debug marker 2: stack switched */
    movl    $0x3fd, %edx
2:  inb     %dx, %al
    testb   $0x20, %al
    jz      2b
    movl    $0x3f8, %edx
    movb    $'2', %al
    outb    %al, %dx

    /* arg1 (rdi) = framebuffer request (Limine filled in ->response) */
    lea   framebuffer_request(%rip), %rdi

    call  kmain                       /* C++ entry, does not return */

3:  cli                               /* safety net: park the CPU */
    hlt
    jmp 3b

/* dbg_mark: raw COM1 byte for C-side debugging (M0 diagnostics) */
.global dbg_mark
dbg_mark:
    movl    $0x3fd, %edx
4:  inb     %dx, %al
    testb   $0x20, %al
    jz      4b
    movl    %edi, %eax
    movl    $0x3f8, %edx
    outb    %al, %dx
    ret

.section .bss.boot_stack, "aw", @nobits
.align 16
.global boot_stack_top
boot_stack_bottom:
    .space 16384
boot_stack_top:
