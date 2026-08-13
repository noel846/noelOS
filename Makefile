all: os.bin

boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin

kernel_asm.o: kernel.asm
	nasm -f elf32 kernel.asm -o kernel_asm.o

kernel.o: kernel.c
	gcc -ffreestanding -m32 -fno-pic -fno-pie -mgeneral-regs-only -c kernel.c -o kernel.o

kernel.bin: kernel_asm.o kernel.o
	ld -m elf_i386 -T link.ld kernel_asm.o kernel.o -o kernel.tmp
	objcopy -O binary kernel.tmp kernel.bin

# 8 here must match `mov al, 8` (sectors to load) in boot.asm's loaddisk.
# Never shrink os.bin below this - that silently truncates the kernel binary.
# Keep AL as small as comfortably fits kernel.bin - large AL values can make
# a single INT 13h CHS read hang instead of erroring (see CLAUDE.md).
os.bin: boot.bin kernel.bin
	cat boot.bin kernel.bin > os.bin
	@sz=$$(stat -c%s os.bin); \
	need=$$(( (1 + 8) * 512 )); \
	if [ $$sz -lt $$need ]; then truncate -s $$need os.bin; fi

run: os.bin
	qemu-system-x86_64 -drive format=raw,file=os.bin

clean:
	rm -f *.bin *.o *.tmp
