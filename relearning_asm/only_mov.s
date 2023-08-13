section .text:
    global _start

_start:
    mov rax, rsp
    mov [rax], 10
