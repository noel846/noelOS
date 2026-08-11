# noelOS - Project Handoff

## What this is
A hobby OS built from scratch in x86 assembly and C. The user (Noel) is a complete beginner
learning OS dev step by step. Be the most beginner-friendly tutor possible. Explain everything.

## What currently works
- 16-bit bootloader loads at 0x7C00, clears screen, prints ".", loads kernel from disk
- Switches to 32-bit protected mode (GDT set up, CR0 PE bit set, far jump flushes pipeline)
- Loads 1 sector (512 bytes) from disk to 0x8000 using INT 13h CHS, DL=[bootdrive], AL=1
- Kernel entry (kernel.asm) sets up segment registers (DS/ES/FS/GS/SS = 0x0010) and stack (ESP = 0x90000)
- Calls kernel_main() in C
- kernel_main writes "shalom from c" directly to VGA text memory (0xB8000) using a local volatile pointer
- Screen shows: "shalom from c"

## What does NOT work yet
- Global variables in kernel.c (video, cursor) - they live in .data section which is at binary
  offset 512, but we only load 1 sector so globals are never loaded into memory
- The proper print() function with global video/cursor pointers
- Anything beyond printing

## The next step
Load 2 sectors instead of 1 so the .data section (globals) gets loaded.

In boot.asm, in loaddisk function, change: `mov al, 1` → `mov al, 2`

Then restore kernel.c to use global variables and a proper print() function:

```c
char* video = (char*) 0xB8000;
int cursor = 0;

void print(char* str){
    int i = 0;
    while(str[i] != 0){
        video[cursor] = str[i];
        video[cursor + 1] = 0x0F;
        cursor = cursor + 2;
        i = i + 1;
    }
}

void kernel_main(){
    print("shalom from c");
}
```

## File descriptions
- boot.asm     — 16-bit bootloader (sector 1). Handles screen, disk load, GDT, PM switch
- kernel.asm   — 32-bit kernel entry. Sets up segments/stack, calls _kernel_main
- kernel.c     — C kernel. Currently: direct VGA writes. Goal: proper print() with globals
- link.ld      — Linker script. Places .text at VMA 0x8000 / LMA 0, .data after .text
- .gitignore   — Ignores *.bin build output

## Build commands (run from C:\Users\noel\Documents\VSCode\assembly)
```powershell
nasm -f bin boot.asm -o boot.bin
nasm -f win32 kernel.asm -o kernel_asm.obj
gcc -ffreestanding -c kernel.c -o kernel.o
ld -T link.ld kernel_asm.obj kernel.o -o kernel.tmp
objcopy -O binary kernel.tmp kernel.bin
$b = [System.IO.File]::ReadAllBytes("boot.bin")
$k = [System.IO.File]::ReadAllBytes("kernel.bin")
$out = New-Object byte[] ($b.Length + $k.Length)
[System.Buffer]::BlockCopy($b, 0, $out, 0, $b.Length)
[System.Buffer]::BlockCopy($k, 0, $out, $b.Length, $k.Length)
[System.IO.File]::WriteAllBytes("os.bin", $out)
& "C:\Program Files\qemu\qemu-system-x86_64.exe" -drive "format=raw,file=os.bin"
```

## Key technical facts (hard-won, don't repeat these mistakes)
- MinGW ld cannot output binary directly (--oformat binary breaks). Use PE → objcopy approach.
- `OUTPUT_FORMAT(binary)` in link.ld breaks MinGW ld. Keep it absent.
- Merging *(.data) into .text in link.ld breaks things with PE format. Keep .data separate.
- .data section in binary lands at offset 512 (PE pads .text to 512 bytes). With AL=1 we only
  load .text. Need AL=2 to also load .data and make globals accessible.
- String literals ("hello") go into .rdata in MinGW, not .data. Use char[] globals instead.
- NASM standalone int 0x10 calls need BH=0 set explicitly or characters go to wrong screen page.
- Use [es:N] direct VGA writes for diagnostics — more reliable than INT 10h.
- INT 13h CHS hangs if you ask for more sectors than the disk image has. Always check image size.
- INT 13h AH=0x42 (LBA) hangs in this QEMU setup. Stick with AH=0x02 (CHS).
- QEMU command must be: `& "C:\Program Files\qemu\qemu-system-x86_64.exe" -drive "format=raw,file=os.bin"`
  (quote the -drive value to prevent PowerShell splitting at commas; use relative path from assembly dir)
- Windows PATH: NASM at C:\Program Files\NASM\, MinGW at C:\MinGW\bin\, QEMU at C:\Program Files\qemu\
- PowerShell binary concat: use [System.IO.File]::ReadAllBytes / WriteAllBytes, NOT copy /b

## User context
- Complete beginner, this is first real project beyond hello world
- On Windows school laptop (MinGW toolchain). Also has Ubuntu at home.
- Wants to eventually write most of the OS in C
- Wants step-by-step explanations of everything
- GitHub repo: https://github.com/noel846/noelOS (private)
