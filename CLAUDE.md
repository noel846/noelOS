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
  - `print()` with global `video`/`cursor` — supports `\n` and `\b` (backspace). **Does not
    scroll yet** — `scroll()` exists (shifts all rows up one, clears the last row) but is
    never called from `print()`, so typing past the bottom of the screen currently just
    writes into off-screen VGA memory instead of scrolling. This is the obvious next fix.
  - `clear()` fills the screen with the current `color`
  - VGA color support: `color` is a global attribute byte (fg | bg<<4)
  - `print_int()` — prints signed decimal integers (handles 0 and negatives)
  - `malloc()` — a bump allocator only (`heap` pointer starts at 0x100000 and just advances by
    `size` each call, no `free()`, no bounds/OOM checking)
  - Interrupts are wired up: PIC remapped (`pic_remap()`), IDT built (`idt[256]`,
    `set_idt_gate()`, `load_idt()`), `sti` enabled in `kernel_main`
  - `timer_handler` (IDT gate 0x20) runs off the PIT (`init_pit(100)` = 100 Hz) and increments
    a global `ticks` counter — this IRQ (IRQ0) is unmasked and actually firing
  - `keyboard_handler` (IDT gate 0x21) exists, filters key-release codes, sets `pending_key` —
    but **IRQ1 is currently masked** (`kernel_main` does `outb(0x21, 0xFE)` after `init_pit`,
    which enables only IRQ0/timer and disables IRQ1/keyboard). So the keyboard interrupt path
    is built but inert; `getkey()` (the original polling version) is still what actually drives
    the shell, deliberately avoiding the getkey()-vs-ISR conflict we identified. `pending_key`
    is also unused right now. Unmasking IRQ1 and switching the shell loop over to consume
    `pending_key` instead of polling is unfinished work.
  - PS/2 keyboard: `inb()`/`outb()` inline asm port I/O, `scancode_to_ascii[]` lookup table,
    `getkey()` busy-waits on port 0x64 status + reads port 0x60 (still the live input path)
  - A shell in `kernel_main`: reads a line via `getkey()`, echoes it, runs `run_command()` on
    Enter. `run_command()` splits `input` into `cmd`/`arg` and dispatches on `cmd`. Commands:
    `hello`, `clear`, `bgcol <color>`, `txcol <color>` (colors: black/blue/green/red/white via
    `color_from_name()`), `uptime` (prints `ticks`), `memtest` (mallocs 16 bytes, writes/prints
    a string into it)
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
gcc -ffreestanding -m32 -fno-pic -fno-pie -mgeneral-regs-only -c kernel.c -o kernel.o
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
- GCC's `__attribute__((interrupt))` is used for ISRs instead of hand-written asm stubs —
  auto-generates register save/restore and `iret`. Requires `-mgeneral-regs-only` (added to
  the Makefile) to stop GCC from touching FPU/SSE registers the attribute doesn't save.
  Every ISR **must** call `outb(0x20, 0x20)` (EOI) unconditionally on every invocation,
  including for scancodes/events you otherwise ignore — skipping it stalls that IRQ line
  forever since the PIC thinks the interrupt is still being handled.
- PIC IRQ mask register (port 0x21 master / 0xA1 slave): bit=1 means that IRQ is **disabled**,
  bit=0 means enabled. Easy to get backwards. Currently keyboard (IRQ1) is masked off on
  purpose — see keyboard_handler note above.
- Keyboard polling (`getkey()`) and the keyboard IRQ handler both drain the same PS/2 output
  buffer (port 0x60) — running both live at once means they race for every keystroke and one
  starves the other. Don't unmask IRQ1 without also switching the shell off `getkey()` polling.

## Roadmap / next steps (rough order)
1. Wire `print()` to call `scroll()` once the cursor passes the last row — `scroll()` already
   exists and is correct, it's just never invoked
2. Finish the keyboard IRQ switchover: unmask IRQ1, replace `getkey()`'s polling loop with
   something that consumes `pending_key` (already being set by keyboard_handler, currently
   unused) — likely needs a `hlt`-based idle loop in `kernel_main` instead of busy-polling
3. Longer-term: proper `free()`/bounds-checked allocator (current `malloc` is a bump allocator
   that never frees), multitasking (now has a timer tick to build a scheduler on top of),
   simple filesystem

## User context
- Complete beginner, this is first real project beyond hello world
- Works from two machines: Windows school laptop (MinGW) and Ubuntu at home — always check
  git sync state first, don't assume the other machine's work is present
- Wants to eventually write most of the OS in C
- Wants step-by-step explanations of everything
- GitHub repo: https://github.com/noel846/noelOS (private)
