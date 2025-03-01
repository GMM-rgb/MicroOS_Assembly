; display_functions.asm - x86-64 implementation
; Compile with NASM for x86-64 systems

section .data
    tick dq 0               ; Define tick as a 64-bit integer (quadword) initialized to 0

section .text
    global initialize_display_memory
    global update_display

initialize_display_memory:
    push rbp
    mov rbp, rsp
    push rbx
    xor rbx, rbx

.init_loop:
    mov byte [rdi + rbx], 0
    inc rbx
    cmp rbx, 64
    jl .init_loop

    pop rbx
    pop rbp
    ret

update_display:
    push rbp
    mov rbp, rsp
    push rbx

    ; Load and increment tick using RIP-relative addressing
    lea rbx, [rel tick]
    mov rax, [rbx]
    inc rax
    mov [rbx], rax

    xor rbx, rbx

.update_loop:
    mov al, [rdi + rbx]
    inc al
    mov [rdi + rbx], al

    mov rax, rbx
    add rax, [rbx]          ; Use 64-bit addressing for tick
    xor rdx, rdx
    mov rcx, 5
    div rcx

    cmp rdx, 0
    jne .skip_variation

    mov al, [rdi + rbx]
    add al, 10
    mov [rdi + rbx], al

.skip_variation:
    inc rbx
    cmp rbx, 64
    jl .update_loop

    pop rbx
    pop rbp
    ret

; section .note.GNU-stack noalloc noexec nowrite progbits
