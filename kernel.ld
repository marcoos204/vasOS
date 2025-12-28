ENTRY(boot) /* if your elf loader can't look into the elf header, this is useless (microcontrollers or plain binary executables) */

SECTIONS {

    . = 0x80200000;

    .text :{
        KEEP(*(.text.boot)); /*this section will be the first one placed in the file*/
        *(.text .text.*);
    }

    .rodata : ALIGN(4) {
        *(.rodata .rodata.*);
    }

    .data : ALIGN(4) {
        *(.data .data.*);
    }

    .bss : ALIGN(4) {
        __bss = .;
        *(.bss .bss.* .sbss .sbss.*);
        __bss_end = .;
    }

    . = ALIGN(4); /* align4 is needed to align to word size */
    . += 128 * 1024;
    __stack_top = .;






}