bits 16

mov ax, 0x07C0
mov ds, ax
mov [bootdrive], dl
mov ax, 0x7E0
mov ss, ax
mov sp, 0x2000

call clearscreen

push 0x0000
call movecursor
add sp, 2

push msg
call print
add sp, 2

call loaddisk

cli
lgdt [gdt_descriptor]

mov eax, cr0
or eax, 1
mov cr0, eax

jmp 0x0008:0x08000

cli
hlt

clearscreen:
    push bp
    mov bp, sp
    pusha

    mov ah, 0x07
    mov al, 0x00
    mov bh, 0x07
    mov cx, 0x00
    mov dh, 0x18
    mov dl, 0x4f
    int 0x10

    popa
    mov sp, bp
    pop bp
    ret

movecursor:
    push bp
    mov bp, sp
    pusha

    mov dx, [bp+4]
    mov ah, 0x02
    mov bh, 0x00
    int 0x10

    popa
    mov sp, bp
    pop bp
    ret

loaddisk:
    push es
    xor ax, ax
    mov es, ax
    mov ah, 0x02
    mov al, 2
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [bootdrive]
    mov bx, 0x8000
    int 0x13
    pop es
    ret

print:
   push bp
   mov bp, sp
   pusha
   mov si, [bp+4]
   mov bh, 0x00
   mov bl, 0x00
   mov ah, 0x0E
.char:
   mov al, [si]
   add si, 1
   or al, 0
   je .return
   int 0x10
   jmp .char
.return:
    popa
    mov sp, bp
    pop bp
    ret


bootdrive: db 0

msg: db ".", 0

gdt_start:
    dq 0
gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00
gdt_end:
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start + 0x7C00




times 510-($-$$) db 0
dw 0xAA55