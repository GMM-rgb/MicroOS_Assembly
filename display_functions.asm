; display_functions.asm - ARM64 implementation
; Compile with NASM for ARM64 systems

.global _initialize_display_memory
.global _update_display
.global _tick

.data
_tick: .quad 0                  // 64-bit counter

.text
_initialize_display_memory:     // void initialize_display_memory(unsigned char *display_memory)
    stp     x29, x30, [sp, #-16]!  // Save frame pointer and link register
    mov     x29, sp
    mov     x1, #0              // Initialize counter

1:  strb    w1, [x0, x1]       // Store byte value at display_memory[i]
    add     x1, x1, #1         // Increment counter
    cmp     x1, #64            // Compare with array size
    b.lt    1b                 // Loop if less than 64

    ldp     x29, x30, [sp], #16    // Restore frame pointer and link register
    ret

_update_display:               // void update_display(unsigned char *display_memory)
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp
    
    adrp    x2, _tick@PAGE     // Load address of tick
    add     x2, x2, _tick@PAGEOFF
    ldr     x3, [x2]          // Load tick value
    add     x3, x3, #1        // Increment tick
    str     x3, [x2]          // Store new tick value

    mov     x1, #0            // Initialize counter

1:  ldrb    w2, [x0, x1]     // Load current byte
    add     w2, w2, #1       // Increment value
    strb    w2, [x0, x1]     // Store updated byte

    add     x4, x1, x3       // i + tick
    mov     x5, #5
    udiv    x6, x4, x5       // (i + tick) / 5
    msub    x6, x6, x5, x4   // Calculate remainder

    cbz     x6, 2f           // If remainder is 0, apply variation
    b       3f               // Skip variation

2:  ldrb    w2, [x0, x1]     // Load current byte again
    add     w2, w2, #10      // Add variation
    strb    w2, [x0, x1]     // Store varied byte

3:  add     x1, x1, #1       // Increment counter
    cmp     x1, #64          // Compare with array size
    b.lt    1b              // Loop if less than 64

    ldp     x29, x30, [sp], #16
    ret
