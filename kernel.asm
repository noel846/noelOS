bits 32

mov ax, 0x0010
mov ds, ax

mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax
mov esp, 0x90000

extern kernel_main
call kernel_main

hang:
    jmp hang

extern timer_handler
global isr_timer
isr_timer:
    pusha
    call timer_handler
    popa
    iret

extern keyboard_handler
global isr_keyboard
isr_keyboard:
    pusha
    call keyboard_handler
    popa
    iret

section .note.GNU-stack noalloc noexec nowrite progbits
