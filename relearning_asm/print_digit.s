; An attempt at writing my own version of an "integer to string" subroutine
; Limitations:
;   - Input: rdi - value
;   - Output: rax - address of string
;             rsi - length (including '\n\0')
%macro exit 1
mov rax, 60
mov rdi, %1
syscall
%endmacro

DIGIT_BUFFER_SIZE equ 22 ; max length of a 64bit, base10 integer

section .bss
    digit_buffer resb DIGIT_BUFFER_SIZE

section .text
    global _start

_start:
    mov rdi, 18446744073709551615
    call digit_to_string

    mov rdi, 1
    mov rdx, rsi
    mov rsi, rax

    mov rax, 1
    syscall

    exit rdx

digit_to_string:
    ; end string with '\n\0'
    lea rcx, [digit_buffer + DIGIT_BUFFER_SIZE - 1]
    mov byte [rcx], 0
    dec rcx
    mov byte [rcx], 10

    mov rsi, 10
    mov rax, rdi
.digit_to_string_loop:
    xor rdx, rdx ; https://stackoverflow.com/questions/38416593/why-should-edx-be-0-before-using-the-div-instruction
    div rsi
    add rdx, 48

    dec rcx
    mov [rcx], dl

    cmp rax, 0
    jne .digit_to_string_loop

    ; calclulate the length of the sting and store the result in rsi
    mov rax, rcx
    lea rsi, [digit_buffer + DIGIT_BUFFER_SIZE - 1]
    sub rsi, rcx
    ret
