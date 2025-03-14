.arch arm64
    .text
    .global _initialize_display_memory
    .type  _initialize_display_memory, %function
_initialize_display_memory:
    sub     sp, sp, #16
    stp     x29, x30, [sp]
    mov     x29, sp
    mov     x1, #0              // Loop counter i = 0
1:
    // Compute value = i * 4
    mov     x2, x1
    mov     x3, #4
    mul     x2, x2, x3
    // Write lower byte of result to display_memory[i]
    uxtb    w2, x2
    strb    w2, [x0, x1]
    add     x1, x1, #1
    cmp     x1, #64
    b.lt    1b
    ldp     x29, x30, [sp]
    add     sp, sp, #16
    ret

    .global _update_display
    .type  _update_display, %function
_update_display:
    sub     sp, sp, #16
    stp     x29, x30, [sp]
    mov     x29, sp
    // Use a global tick variable stored in memory
    adrp    x2, _tick
    add     x2, x2, :lo12:_tick
    ldr     x3, [x2]
    add     x3, x3, #1
    str     x3, [x2]
    mov     x1, #0              // Loop counter i = 0
2:
    ldrb    w4, [x0, x1]
    add     w4, w4, #1         // Increment byte value
    strb    w4, [x0, x1]
    add     x1, x1, #1
    cmp     x1, #64
    b.lt    2b
    ldp     x29, x30, [sp]
    add     sp, sp, #16
    ret

    .data
_tick:
    .quad   0
