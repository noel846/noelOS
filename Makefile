all: os.bin

boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin

kernel_asm.o: kernel.asm
	nasm -f elf32 kernel.asm -o kernel_asm.o

kernel.o: kernel.c
	gcc -ffreestanding -m32 -fno-pic -fno-pie -c kernel.c -o kernel.o

kernel.bin: kernel_asm.o kernel.o
	ld -m elf_i386 -T link.ld kernel_asm.o kernel.o -o kernel.tmp
	objcopy -O binary kernel.tmp kernel.bin

os.bin: boot.bin kernel.bin
	cat boot.bin kernel.bin > os.bin
	truncate -s 4096 os.bin

run: os.bin
	qemu-system-x86_64 -drive format=raw,file=os.bin

clean:
	rm -f *.bin *.o *.tmp
