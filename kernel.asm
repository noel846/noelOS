bits 32

mov ax, 0x0010
mov ds, ax

mov edi, 0xB8000
mov byte [edi], 'H'
mov byte [edi+1], 0x0F

cli
hlt
