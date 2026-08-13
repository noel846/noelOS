# noelOS - Project Handoff

## What this is
A hobby OS built from scratch in x86 assembly and C. The user (Noel) is a complete beginner
learning OS dev step by step. Be the most beginner-friendly tutor possible. Explain everything.

## What currently works
- 16-bit bootloader (boot.asm) loads at 0x7C00, clears screen, prints ".", loads the kernel
  from disk, switches to 32-bit protected mode (GDT set up, CR0 PE bit set, far jump flushes
  pipeline), jumps to 0x8000
- Loads 8 sectors (4096 bytes) from disk to 0x8000 using INT 13h CHS, DL=[bootdrive], AL=8,
  starting at CL=2. kernel.bin is currently ~1.4KB, so this has room to grow before AL needs
  bumping again - see "kernel-size vs disk-load-size" fact below before touching this number.
- Kernel entry (kernel.asm) sets up segment registers (DS/ES/FS/GS/SS = 0x0010) and stack
  (ESP = 0x90000), calls kernel_main() in C
- kernel.c has: global video/cursor pointers, print() (handles \n and \b), clear(), a PS/2
  keyboard driver (inb + scancode-to-ascii table + getkey()), and a tiny shell (run_command)
  with commands: hello, clear, bgcol red/black, txcol blue/white
- Boots into a "noelOS" prompt where you can type commands and press Enter (confirmed working
  on Windows/MinGW on 2026-08-13)

## Repo has two build environments - they are NOT interchangeable
The user works from both a Windows laptop (MinGW toolchain) and an Ubuntu machine. The checked-in
`Makefile` was written on the Ubuntu side and uses `nasm -f elf32` / `ld -m elf_i386` / `cat` /
`truncate` - **this does not work on Windows.** MinGW's `ld` on this machine only supports the
`i386pe` emulation, not `elf_i386` (confirmed: `ld -m elf_i386` errors with "unrecognised
emulation mode"). On Windows, `make` itself isn't even installed. Build manually on Windows using
the PowerShell recipe below (PE/win32 objects + objcopy, not ELF).

There is also a **symbol name mangling mismatch** between the two toolchains: MinGW's gcc prefixes
C symbols with a leading underscore (`kernel_main` -> `_kernel_main`), Linux's gcc (ELF) does not.
kernel.asm calls `kernel_main` with no underscore (written on the Linux side). On Windows this
needs `gcc -fno-leading-underscore` to make the symbol names match, or the link fails with
"undefined reference to `kernel_main`". Do NOT edit kernel.asm to add the underscore back -
that would break the Linux build instead.

There is a **third** MinGW-only gotcha, in link.ld - see the ".rdata$zzz orphan section" fact
below. It's already fixed in the checked-in link.ld; just don't undo it.

## Build commands - Windows (PowerShell, run from C:\Users\noel\Documents\VSCode\assembly)
```powershell
nasm -f bin boot.asm -o boot.bin
nasm -f win32 kernel.asm -o kernel_asm.o
gcc -ffreestanding -m32 -fno-pic -fno-pie -fno-leading-underscore -c kernel.c -o kernel.o
ld -T link.ld kernel_asm.o kernel.o -o kernel.tmp
objcopy -O binary kernel.tmp kernel.bin
$b = [System.IO.File]::ReadAllBytes("boot.bin")
$k = [System.IO.File]::ReadAllBytes("kernel.bin")
$need = (1 + 8) * 512   # 1 boot sector + the 8 sectors boot.asm's loaddisk reads
$out = New-Object byte[] ([Math]::Max($b.Length + $k.Length, $need))
[System.Buffer]::BlockCopy($b, 0, $out, 0, $b.Length)
[System.Buffer]::BlockCopy($k, 0, $out, $b.Length, $k.Length)
[System.IO.File]::WriteAllBytes("os.bin", $out)
& "C:\Program Files\qemu\qemu-system-x86_64.exe" -drive "format=raw,file=os.bin"
```

## Build commands - Ubuntu / Linux (`make` or `make run`)
Uses the checked-in Makefile as-is (elf32 objects, `cat`/`truncate`). Do not port the Windows
flags (`-fno-leading-underscore`, `-f win32`) over here - they're Windows-only workarounds.

## Key technical facts (hard-won, don't repeat these mistakes)

- **The kernel-size vs disk-load-size bug (2026-08-13):** kernel.c grew a lot in one pull (13
  lines to 124: shell, keyboard driver, etc.) and kernel.bin outgrew what boot.asm was loading
  (AL=4 sectors = 2048 bytes for an 8216-byte kernel) - kernel ran on garbage/truncated code.
  Made worse by the Makefile's old `truncate -s 4096 os.bin`, which *shrinks* the image and
  silently chops the kernel binary if it's bigger than 4096 bytes. **Whenever kernel.c grows a
  lot, check kernel.bin's size against `AL * 512` in boot.asm's loaddisk - if kernel.bin is close
  to or over that, bump AL and update the matching `need`/truncate size in both the Makefile and
  the PowerShell recipe.** Don't jump AL up by a lot "just in case" - see the next fact for why.

- **AL sector count: bigger is not safer (2026-08-13):** after fixing the bug above, AL was
  bumped to 62 (reasoning: sectors 2-63 stay within one BIOS-translated 63-sectors/track region,
  so a single INT 13h CHS read can't cross a track boundary). This was wrong in practice - it
  made the boot hang completely (screen stuck on the bootloader's "." forever, protected mode
  never entered). Reverting to AL=8 (still comfortably more than the ~1.4KB kernel needs) fixed
  it immediately. This matches the older, already-documented lesson below ("INT 13h CHS hangs if
  you ask for more sectors than the disk image has") - it's not fully understood *why* 62 hangs
  in this QEMU/BIOS setup while 8 doesn't, just that it does. **Keep AL as small as comfortably
  covers kernel.bin's size (e.g. current size in sectors, rounded up, plus a few sectors of
  headroom) - do not jump to a large "safe-looking" number.** If kernel.bin ever needs more than
  ~30 sectors, increase AL gradually (e.g. +8-16 at a time) and re-test booting after each bump,
  rather than jumping straight to something like 62.

- **MinGW's `.rdata$zzz` orphan section (2026-08-13):** MinGW's PE/COFF gcc splits read-only
  data into multiple named subsections (e.g. `.rdata` AND `.rdata$zzz` - the `$zzz` suffix is a
  COMDAT-folding convention specific to PE/COFF; ELF/Linux doesn't do this). The old link.ld only
  matched `*(.rdata)` (exact name), so `.rdata$zzz` became an unmatched "orphan" section that the
  linker placed at its own address, which also silently broke the `AT(ADDR(.data) - 0x8000)` LMA
  math for every section after it. Symptom: kernel.bin bloated to 8x its expected size and the
  kernel never ran because .data/.rodata ended up loaded at the wrong runtime address. Fixed by
  broadening link.ld's patterns to wildcards: `*(.data) *(.data*) *(.rodata) *(.rodata*)
  *(.rdata) *(.rdata*)`, and adding `*(.drectve)` (linker metadata, not real code/data) to
  /DISCARD/. **If kernel.bin size balloons unexpectedly after adding new string literals/consts
  to kernel.c, check `objdump -h kernel.tmp` for extra orphan sections with VMA/LMA that don't
  look contiguous with .text/.data - that's this bug again.**
  Note: this MinGW backend also does NOT honor AT() to produce a real ELF-style LMA != VMA split
  (PE/COFF has no separate load-address concept) - it just always sets LMA=VMA. That's fine here
  because `objcopy -O binary` derives file offsets from `address - lowest_address_in_image`, and
  since `.text`'s VMA (0x8000) *is* the lowest address and matches boot.asm's load address, the
  layout comes out correct anyway as long as sections are contiguous (no orphans).

- **How to debug a "boots to nothing" problem without the user's eyes (2026-08-13):** when the
  screen just shows the bootloader's "." and you don't know why, don't guess blindly or dive into
  `-d int` exception logs (huge, noisy, easy to misread - a live snapshot taken mid a crash/hang
  loop can look like nonsense and waste a lot of time). Instead: (1) add one-byte VGA marker
  writes (`mov byte [0xB8000+N], 'X'` in asm, or `video[N]='X'` in C) at each boot stage
  (post-disk-load in boot.asm, kernel.asm entry, kernel_main entry in C) at N far enough apart
  they don't overlap; (2) launch QEMU headless with a TCP monitor:
  `-monitor tcp:127.0.0.1:PORT,server,nowait -display none -no-reboot -no-shutdown`; (3) from
  Python (no special libs needed - `socket` is enough; this machine's Python has no `AF_UNIX`,
  use TCP not a unix socket), connect and send `screendump <path>.ppm\n`; (4) either eyeball the
  .ppm (rare tools open it directly) or scan it programmatically - VGA text mode at 720x400 is an
  80x25 grid of 9x16-pixel cells, so `cell_bright = any pixel in that 9x16 block above a
  brightness threshold` tells you which character cells got written to, without needing to
  actually render/OCR glyphs. Whichever marker is the last one lit tells you exactly which stage
  broke. This found the AL=62 hang directly (marker after loaddisk lit, marker at kernel.asm
  entry never lit -> problem is the disk read or the protected-mode jump, not the C code).
  `info registers` / `-d int` logs are much less reliable for this than they sound - QEMU's
  vCPU can be mid-instruction relative to when the monitor query lands, especially without
  hardware acceleration (KVM), and boot can take longer in wall-clock time than expected, so a
  register snapshot can look like it's stuck in a totally different place than it really is.
  screendumps of the final settled state are far more trustworthy signal.

- MinGW ld cannot output binary directly (--oformat binary breaks). Use PE → objcopy approach.
- `OUTPUT_FORMAT(binary)` in link.ld breaks MinGW ld. Keep it absent.
- Merging *(.data) into .text in link.ld breaks things with PE format. Keep .data separate.
- .data section in binary lands right after .text. Loading only enough sectors for .text means
  globals never reach memory - this is why we load more sectors than .text alone needs.
- String literals ("hello") go into .rdata in MinGW, not .data. Use char[] globals instead. (And
  now that link.ld's `*(.rdata*)` wildcard is in place, .rdata's contents actually get merged
  into the loaded .data output section - see the `.rdata$zzz` fact above.)
- NASM standalone int 0x10 calls need BH=0 set explicitly or characters go to wrong screen page.
- Use [es:N] / [0xB8000+N] direct VGA writes for diagnostics — more reliable than INT 10h, and
  works in both real mode (via an es segment pointed at 0xB800) and protected mode (flat address).
- INT 13h CHS can hang (not just error) if AL asks for more than the disk image/BIOS geometry can
  service in one call. Keep AL modest and increase gradually - see the AL fact above. Also always
  make sure os.bin is padded to at least (1 + AL) * 512 bytes.
- INT 13h AH=0x42 (LBA) hangs in this QEMU setup. Stick with AH=0x02 (CHS).
- QEMU command must be: `& "C:\Program Files\qemu\qemu-system-x86_64.exe" -drive "format=raw,file=os.bin"`
  (quote the -drive value to prevent PowerShell splitting at commas; use relative path from assembly dir)
- Windows PATH: NASM at C:\Program Files\NASM\, MinGW at C:\MinGW\bin\, QEMU at C:\Program Files\qemu\
- PowerShell binary concat: use [System.IO.File]::ReadAllBytes / WriteAllBytes, NOT copy /b
- Background QEMU processes started for debugging can pile up if not explicitly killed (`taskkill
  //F //PID <pid>`, or `quit` via the monitor connection) - check `tasklist | grep qemu` and clean
  up stray headless instances before finishing a debugging session.

## File descriptions
- boot.asm     — 16-bit bootloader (sector 1). Handles screen, disk load, GDT, PM switch
- kernel.asm   — 32-bit kernel entry. Sets up segments/stack, calls kernel_main
- kernel.c     — C kernel: print()/clear(), PS/2 keyboard driver, tiny shell with color commands
- link.ld      — Linker script. Places .text at VMA 0x8000, .data right after (see hard-won facts
  above for the MinGW-specific wildcard patterns this needs)
- Makefile     — Linux/Ubuntu build only (see "two build environments" above)
- .gitignore   — Ignores *.bin build output

## User context
- Complete beginner, this is first real project beyond hello world
- On Windows school laptop (MinGW toolchain). Also has Ubuntu at home - builds/commits happen
  from both machines, so watch for toolchain-specific breakage when pulling.
- Wants to eventually write most of the OS in C
- Wants step-by-step explanations of everything
- GitHub repo: https://github.com/noel846/noelOS (private)
