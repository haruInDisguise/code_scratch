%macro exit 1
    mov rdi, %1
    mov rax, SYS_EXIT
    syscall
%endmacro

FD_STDOUT equ 1

SYS_EXIT equ 60
SYS_WRITE equ 1

section .data
    some_text db "Hello World!",10,0
    another_text db "This is another text... WOOW", 10, 0

section .text
    global _start

_start:
    mov rdi, some_text
    call print_string

    mov rdi, another_text
    call print_string

    exit 42

; input - rdi
print_string:
    push rdi     ; rdi will be modified by strlen
    call strlen  ; write the length of the string in rdi into rax
    mov rdx, rax ; prepare write syscall
    mov rax, SYS_WRITE
    pop rsi      ; pop rdi into rsi (string address)
    mov rdi, FD_STDOUT
    syscall
    ret

; input - rdi
; output - rax
strlen:
    xor rax, rax
strlen_loop:
    inc rax
    inc rdi
    mov cl, [rdi]
    cmp cl, 0
    jne strlen_loop

    ret
