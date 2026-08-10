bits 16

mov ax, 0x0800
mov ds, ax

push msg
call print
add sp, 2

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


msg: db " kernel loaded", 0