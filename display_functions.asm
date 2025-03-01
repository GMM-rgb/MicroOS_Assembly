; display_functions.asm - x86-64 implementation
; Compile with NASM for x86-64 systems

section .data
    tick dq 0               ; Define tick as a 64-bit integer (quadword) initialized to 0

section .text
    global initialize_display_memory
    global update_display

; void initialize_display_memory(unsigned char *display_memory)
initialize_display_memory:
    push rbp
    mov rbp, rsp

    push rbx                  ; Save RBX (callee-saved register)
    xor rbx, rbx              ; Clear counter

.init_loop:
    mov byte [rdi + rbx], 0   ; Set each byte in display_memory to 0
    inc rbx
    cmp rbx, 64               ; Loop 64 times
    jl .init_loop

    pop rbx
    pop rbp
    ret

; void update_display(unsigned char *display_memory)
update_display:
    push rbp
    mov rbp, rsp

    push rbx                  ; Save RBX
    mov rbx, 0                ; Clear counter

    ; Increment static tick counter
    mov rax, [tick]           ; Load tick
    inc rax                   ; Increment tick
    mov [tick], rax           ; Save updated tick

.update_loop:
    mov al, [rdi + rbx]       ; Load current byte
    inc al                    ; Increment byte
    mov [rdi + rbx], al       ; Store incremented byte

    mov rax, rbx              ; Current index
    add rax, [tick]           ; Add tick value
    xor rdx, rdx
    mov rcx, 5
    div rcx                   ; Divide RAX by 5, remainder in RDX

    cmp rdx, 0
    jne .skip_variation       ; Skip modification if remainder != 0

    mov al, [rdi + rbx]       ; Reload current byte
    add al, 10                ; Add variation
    mov [rdi + rbx], al       ; Store updated byte

.skip_variation:
    inc rbx
    cmp rbx, 64
    jl .update_loop           ; Repeat for all 64 bytes

    pop rbx
    pop rbp
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
