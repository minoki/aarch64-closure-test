#if defined(__APPLE__)
#define C(x) _##x
#else
#define C(x) x
#endif
        .text
        .align 2
        .global C(thunk)
C(thunk):
        mov x10, sp // old args
        stp x29, x30, [sp, #-16]! // save the frame pointer and the link register
        str x28, [sp, #-16]!
        ldr x28, [x16] // space
        sub sp, sp, x28
        mov x9, sp // new args
        stp d6, d7, [sp, #-16]!
        stp d4, d5, [sp, #-16]!
        stp d2, d3, [sp, #-16]!
        stp d0, d1, [sp, #-16]!
        stp x6, x7, [sp, #-16]!
        stp x4, x5, [sp, #-16]!
        stp x2, x3, [sp, #-16]!
        stp x0, x1, [sp, #-16]!
        mov x3, x16 // info
        mov x2, x10 // stack_old
        mov x1, x9 // stack_new
        mov x0, sp // integer registers
        bl C(adjust_args)
        mov x9, x0
        ldp x0, x1, [sp], #16
        ldp x2, x3, [sp], #16
        ldp x4, x5, [sp], #16
        ldp x6, x7, [sp], #16
        ldp d0, d1, [sp], #16
        ldp d2, d3, [sp], #16
        ldp d4, d5, [sp], #16
        ldp d6, d7, [sp], #16
        blr x9
        add sp, sp, x28
        ldr x28, [sp], #16
        ldp x29, x30, [sp], #16 // restore the frame pointer and the link register
        ret
