ENTRY(_start)

SECTIONS
{
    /* Higher half kernel base */
    . = 0xffffffff80000000;

    .requests ALIGN(4K) : {
        KEEP(*(.requests_start_marker))
        KEEP(*(.requests))
        KEEP(*(.requests_end_marker))
    }

    .text ALIGN(4K) : {
        *(.text .text.*)
    }

    .rodata ALIGN(4K) : {
        *(.rodata .rodata.*)
    }

    .data ALIGN(4K) : {
        *(.data .data.*)
    }
    


    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss .bss.*)
    }

    /* Discard unneeded sections */
    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
        *(.comment)
    }
}