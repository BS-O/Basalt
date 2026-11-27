/*
 * Basalt - 64-bit entry stub (L0, pure assembly).
 *
 * Entered by Limine in x86-64 long mode with interrupts disabled and the
 * kernel mapped (see ADR-002). We only set up our own stack and hand over
 * to the C++ kernel entry point.
 *
 * Boot-stage probes from M0 ('1' on entry, '2' after the stack switch) were
 * removed once make check started asserting on the real boot log; kmain still
 * emits a single 'K' marker (dbg_mark) before the console is up.
 */

.section .text.boot, "ax", @progbits
.global boot_entry
boot_entry:
    cli

    /* switch to our own stack (.bss, 16 KiB, 16-byte aligned) */
    movabsq $boot_stack_top, %rsp
    xorq  %rbp, %rbp

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
