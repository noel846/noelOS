SECTIONS {
    . = 0x8000;
    .text : AT(0) { *(.text*) }
    .data : AT(ADDR(.data) - 0x8000) { *(.data) *(.data*) *(.rodata) *(.rodata*) *(.rdata) *(.rdata*) }
    .bss (NOLOAD) : { *(.bss) }
    /DISCARD/ : { *(.eh_frame) *(.got.plt) *(.note.GNU-stack) *(.drectve) }
}
