section .data
    our_string db "Hello World!",10,0

section .text
    global _start

_start:
    mov rax, 1          ; arg ID
    mov rdi, 1          ; arg 0
    mov rsi, our_string ; arg 1
    mov rdx, 15         ; arg 2 syscall

    mov rax, 60      ; exit system call
    mov rdi, 42
    syscall

