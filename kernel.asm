bits 32

mov ax, 0x0010
mov ds, ax

mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax
mov esp, 0x90000

extern _kernel_main
call _kernel_main

.hang:
    jmp .hang
