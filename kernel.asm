bits 32
org 0x8000

mov ax, 0x0010
mov ds, ax

mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax
mov esp, 0x90000

mov edi, 0xB8000
mov esi, msg

.loop:
    mov al, [esi]
    cmp al, 0
    je .done
    mov [edi], al
    mov byte [edi+1], 0x0F
    add esi, 1
    add edi, 2
    jmp .loop
.done:


extern kernel_main
call kernel_main


msg: db "hello from protected mode", 0