# noelOS - Project Handoff

## What this is
A hobby OS built from scratch in x86 assembly and C. The user (Noel) is a complete beginner
learning OS dev step by step. Be the most beginner-friendly tutor possible. Explain everything.
Noel works from two machines — a Windows school laptop (MinGW toolchain) and an Ubuntu machine
at home — and syncs via GitHub. Always check `git status`/`git log` against origin at the start
of a session; work is often pushed from whichever machine was used last.

## What currently works
- 16-bit bootloader (boot.asm) loads at 0x7C00, clears screen, prints ".", sets up GDT,
  switches to 32-bit protected mode, far-jumps to the kernel at 0x8000
- Loads **8 sectors** from disk via INT 13h CHS (AL=8, DL=[bootdrive]) — enough for the
  current kernel.bin. `mov al, 8` in boot.asm's `loaddisk` MUST match the Makefile's
  `os.bin` padding math (`(1 + 8) * 512`). Never shrink below that — it silently truncates
  the kernel binary instead of erroring.
- Kernel entry (kernel.asm) sets flat segments (DS/ES/FS/GS/SS=0x0010), stack at ESP=0x90000,
  calls `kernel_main` (no underscore — see Build below)
- C kernel (kernel.c) has:
  - `print()` with global `video`/`cursor` — supports `\n` and `\b` (backspace)
  - `clear()` fills the screen with the current `color`
  - VGA color support: `color` is a global attribute byte (fg | bg<<4)
  - PS/2 keyboard driver: `inb()` inline asm port read, polls port 0x64 status + 0x60 data,
    `scancode_to_ascii[]` lookup table, `getkey()` busy-waits for a keypress (no IRQs/IDT yet)
  - A basic shell in `kernel_main`: reads a line via `getkey()`, echoes it, runs `run_command()`
    on Enter. Commands so far: `hello`, `clear`, `bgcol red`, `bgcol black`, `txcol blue`,
    `txcol white` (uncommitted local addition in progress: `txcol green`)
  - Command matching is a straight `streq()` chain — no argument parsing yet
- Screen boots into a live shell prompt (`noelOS\n> `) that accepts typed commands

## Build (unified — works on both Windows/MinGW and Ubuntu)
The Makefile now targets **ELF (`elf32`/`elf_i386`)** on both machines, not PE. This was a
deliberate fix (commit "Fix Windows boot hang: MinGW orphan .rdata section and bad AL sector
count") — MinGW's `ld` here does support `-m elf_i386`, so one Makefile works everywhere.
```
make        # builds os.bin
make run    # builds and launches QEMU
make clean
```
Manual steps (what the Makefile does):
```
nasm -f bin boot.asm -o boot.bin
nasm -f elf32 kernel.asm -o kernel_asm.o
gcc -ffreestanding -m32 -fno-pic -fno-pie -c kernel.c -o kernel.o
ld -m elf_i386 -T link.ld kernel_asm.o kernel.o -o kernel.tmp
objcopy -O binary kernel.tmp kernel.bin
cat boot.bin kernel.bin > os.bin   # pad to (1+8)*512 bytes if short — see Makefile
qemu-system-x86_64 -drive format=raw,file=os.bin
```
On Windows, `cat`/`stat`/`truncate` need Git Bash (`make` run under a bash-capable shell) —
plain PowerShell/cmd won't have them.

## File descriptions
- boot.asm     — 16-bit bootloader (sector 1). Screen, disk load (8 sectors), GDT, PM switch
- kernel.asm   — 32-bit kernel entry. Sets up segments/stack, calls `kernel_main`
- kernel.c     — C kernel. print/clear/color, PS/2 keyboard driver, shell + command dispatch
- link.ld      — Linker script (ELF). .text at VMA 0x8000/LMA 0; .data folds in .rodata/.rdata
  to avoid orphan sections; discards .eh_frame/.got.plt/.note.GNU-stack/.drectve
- Makefile     — Unified build for Windows(MinGW)+Ubuntu, ELF-based
- .gitignore   — Ignores build output

## Key technical facts (hard-won, don't repeat these mistakes)
- Build is ELF-based (`elf32`/`elf_i386`) on **both** platforms now — the old PE/`objcopy`-only
  approach and `_kernel_main` underscore prefix are obsolete; don't reintroduce them.
- `.data`/`.rodata`/`.rdata` must be explicitly merged into `.data` in link.ld, and stray
  sections (`.eh_frame`, `.got.plt`, `.note.GNU-stack`, `.drectve`) explicitly discarded —
  otherwise the linker inserts orphan sections that shift layout and hang the boot on Windows.
- `boot.asm`'s `loaddisk` sector count (`mov al, N`) must always be ≥ kernel.bin's real size
  in sectors, and must match the Makefile's padding math. Mismatch = silent truncation or
  INT 13h hang (BIOS CHS reads hang, not error, if asked for more sectors than the image has).
- INT 13h AH=0x42 (LBA) hangs in this QEMU setup. Stick with AH=0x02 (CHS).
- NASM standalone INT 10h calls need BH=0 explicitly or output goes to an invisible page.
- QEMU must be run with `-drive format=raw,file=os.bin` (quote the value in PowerShell to
  avoid comma-splitting), from the project directory.
- Windows PATH: NASM at C:\Program Files\NASM\, MinGW at C:\MinGW\bin\, QEMU at
  C:\Program Files\qemu\
- Keyboard input is currently **polled** (busy-wait on port 0x64), not interrupt-driven —
  no IDT/PIC setup yet. Fine for a single-threaded shell but will need to change once timers
  or multitasking are added.

## Roadmap / next steps (rough order)
1. Finish/clean up shell commands — argument parsing instead of one `streq()` per exact string
   (e.g. split command + args so `txcol <name>` and `bgcol <name>` share one handler)
2. Number printing (`print_int`/basic printf) — needed before anything reports a status or count
3. IDT + PIC remap + interrupt-driven keyboard (replaces the polling loop) — foundational for
   everything below
4. PIT timer interrupt — system tick / uptime, needed for any future scheduling
5. Memory management — a simple heap allocator (`malloc`-equivalent)
6. Longer-term: multitasking, simple filesystem

## User context
- Complete beginner, this is first real project beyond hello world
- Works from two machines: Windows school laptop (MinGW) and Ubuntu at home — always check
  git sync state first, don't assume the other machine's work is present
- Wants to eventually write most of the OS in C
- Wants step-by-step explanations of everything
- GitHub repo: https://github.com/noel846/noelOS (private)
