; ============================================================================
; lowl Compiler v2.1.0 - Complete Implementation
; Copyright (c) 2026 Anthony Matarazzo
; Licensed under MIT License
; System V AMD64 ABI
; ============================================================================

bits 64
section .text
global main


; Function: in_i8 (opt: 2)
in_i8:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    mov qword [rbp + -8], rdi
    mov rax, qword [rbp + -8]
    push rax
    mov rax, 128
    push rax
    pop rax
    neg rax
    push rax
    pop rbx
    pop rax
    cmp rax, rbx
    setge al
    movzx eax, al
    push rax
    mov rax, qword [rbp + -8]
    push rax
    mov rax, 127
    push rax
    pop rbx
    pop rax
    cmp rax, rbx
    setle al
    movzx eax, al
    push rax
    pop rbx
    pop rax
    test rax, rax
    setne al
    movzx eax, al
    test rbx, rbx
    setne bl
    movzx ebx, bl
    and rax, rbx
    push rax
    pop rax
    test rax, rax
    jz .L1
    mov rax, 1
    push rax
    pop rax
    mov rsp, rbp
    pop rbp
    ret
    jmp .L0
.L1:
.L0:
    mov rax, 0
    push rax
    pop rax
    mov rsp, rbp
    pop rbp
    ret
    xor eax, eax
    mov rsp, rbp
    pop rbp
    ret

; Function: main (opt: 2)
main:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    mov rax, 127
    push rax
    pop rdi
    call in_i8
    push rax
    pop rdi
    call print_u64
    push rax
    lea rax, [rel .LCs3]
    push rax
    pop rdi
    call print_string
    push rax
    mov rax, 128
    push rax
    pop rdi
    call in_i8
    push rax
    pop rdi
    call print_u64
    push rax
    lea rax, [rel .LCs4]
    push rax
    pop rdi
    call print_string
    push rax
    mov rax, -128
    push rax
    pop rdi
    call in_i8
    push rax
    pop rdi
    call print_u64
    push rax
    lea rax, [rel .LCs5]
    push rax
    pop rdi
    call print_string
    push rax
    mov rax, -129
    push rax
    pop rdi
    call in_i8
    push rax
    pop rdi
    call print_u64
    push rax
    lea rax, [rel .LCs6]
    push rax
    pop rdi
    call print_string
    push rax
    mov rax, -7
    push rax
    pop rax
    mov qword [rbp + -8], rax
    mov rax, qword [rbp + -8]
    push rax
    pop rdi
    call print_i64
    push rax
    lea rax, [rel .LCs7]
    push rax
    pop rdi
    call print_string
    push rax
    mov rax, qword [rbp + -8]
    push rax
    mov rax, 0
    push rax
    pop rbx
    pop rax
    cmp rax, rbx
    setl al
    movzx eax, al
    push rax
    pop rdi
    call print_u64
    push rax
    lea rax, [rel .LCs8]
    push rax
    pop rdi
    call print_string
    push rax
    mov rax, 0
    push rax
    pop rax
    mov rsp, rbp
    pop rbp
    ret
    xor eax, eax
    mov rsp, rbp
    pop rbp
    ret

section .rodata
.LCs3: db 32, 0
.LCs4: db 32, 0
.LCs5: db 32, 0
.LCs6: db 10, 0
.LCs7: db 32, 0
.LCs8: db 10, 0

