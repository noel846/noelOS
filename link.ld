SECTIONS {
    . = 0x8000;
    .text : AT(0) { *(.text) }
    .data : AT(LOADADDR(.text) + SIZEOF(.text)) { *(.data) }
    .bss (NOLOAD) : { *(.bss) }
}
