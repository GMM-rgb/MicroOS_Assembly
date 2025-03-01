.section __DATA,__data
.align 3
.globl _tick
_tick:  .quad 0

.section __TEXT,__text
.align 2

.globl _initialize_display_memory
_initialize_display_memory:
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp
    mov     x1, #0              // counter = 0
1:  strb    w1, [x0, x1]       // store byte at display_memory[counter]
    add     x1, x1, #1         // counter++
    cmp     x1, #64            // compare counter with 64
    b.lt    1b                 // loop if counter < 64
    ldp     x29, x30, [sp], #16
    ret

.globl _update_display
_update_display:
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp
    
    adrp    x2, _tick@PAGE
    add     x2, x2, _tick@PAGEOFF
    ldr     x3, [x2]           // load tick
    add     x3, x3, #1         // increment tick
    str     x3, [x2]           // store tick
    
    mov     x1, #0             // counter = 0
1:  ldrb    w2, [x0, x1]       // load current byte
    add     w2, w2, #1         // increment value
    strb    w2, [x0, x1]       // store updated byte
    
    add     x4, x1, x3         // x4 = counter + tick
    mov     x5, #5
    udiv    x6, x4, x5         // divide by 5
    msub    x6, x6, x5, x4     // get remainder
    
    cbnz    x6, 2f             // skip if remainder != 0
    ldrb    w2, [x0, x1]       // load byte again
    add     w2, w2, #10        // add variation
    strb    w2, [x0, x1]       // store varied byte
    
2:  add     x1, x1, #1         // counter++
    cmp     x1, #64            // compare counter with 64
    b.lt    1b                 // loop if counter < 64
    
    ldp     x29, x30, [sp], #16
    ret
