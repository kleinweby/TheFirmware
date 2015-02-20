MEMORY
{
  /* Define each memory region */
  MFlash16 (rx) : ORIGIN = 0x0, LENGTH = 0x8000 /* 32k */
  RamLoc8 (rwx) : ORIGIN = 0x100000b8, LENGTH = 0x1F48 /* ORIGIN = 0x10000000, LENGTH = 0x2000 /* 8k */

}
  /* Define a symbol for the top of each memory region */
  __top_MFlash16 = 0x0 + 0x8000;
  __top_RamLoc = 0x10000000 + 0x2000;

SECTIONS
{
    /* MAIN TEXT SECTION */
    .text : ALIGN(4)
    {
        FILL(0xff)
        __vectors_start__ = ABSOLUTE(.) ;
        KEEP(*(.isr_vector))

        /* Global Section Table */
        . = ALIGN(4) ;
        __section_table_start = .;
        __data_section_table = .;
        LONG(LOADADDR(.data));
        LONG(    ADDR(.data)) ;
        LONG(  SIZEOF(.data));
        __data_section_table_end = .;
        __bss_section_table = .;
        LONG(    ADDR(.bss));
        LONG(  SIZEOF(.bss));
        __bss_section_table_end = .;
        __section_table_end = . ;
        /* End of Global Section Table */


        *(.after_vectors*)

    } >MFlash16

    .text : ALIGN(4)
    {
         *(.text*)
        . = ALIGN(4);

        /* C++ constructors etc */
        . = ALIGN(4);
        KEEP(*(.init))

        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP (*(.preinit_array))
        __preinit_array_end = .;

        . = ALIGN(4);
        __init_array_start = .;
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        __init_array_end = .;

        KEEP(*(.fini));

        . = ALIGN(4);
        KEEP (*crtbegin.o(.ctors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
        KEEP (*(SORT(.ctors.*)))
        KEEP (*crtend.o(.ctors))

        . = ALIGN(4);
        KEEP (*crtbegin.o(.dtors))
        KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
        KEEP (*(SORT(.dtors.*)))
        KEEP (*crtend.o(.dtors))
        /* End C++ */
    } > MFlash16


    .text : ALIGN(4) {
        PROVIDE_HIDDEN(_fw_init_begin = .);

        KEEP(*(SORT(._fw_init.*)))
        KEEP(*(.fw_init))

        PROVIDE_HIDDEN(_fw_init_end = .);

        PROVIDE_HIDDEN(_console_cmds_begin = .);

        KEEP(*(SORT(.console_cmds.*)))
        KEEP(*(.console_cmds))

        PROVIDE_HIDDEN(_console_cmds_end = .);
    } > MFlash16

    .rodata : ALIGN(4) {
                *(.rodata .rodata.*)

    } > MFlash16


    /*
     * for exception handling/unwind - some Newlib functions (in common
     * with C++ and STDC++) use this.
     */
    .ARM.extab : ALIGN(4)
    {
    	*(.ARM.extab* .gnu.linkonce.armextab.*)
    } > MFlash16
    __exidx_start = .;

    .ARM.exidx : ALIGN(4)
    {
    	*(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > MFlash16
    __exidx_end = .;

    _etext = .;


    /* MAIN DATA SECTION */


    .uninit_RESERVED : ALIGN(4)
    {
        KEEP(*(.bss.$RESERVED*))
        . = ALIGN(4) ;
        _end_uninit_RESERVED = .;
    } > RamLoc8


	/* Main DATA section (RamLoc8) */
	.data : ALIGN(4)
	{
	   FILL(0xff)
	   _data = . ;
	   *(vtable)
	   *(.data*)
	   . = ALIGN(4) ;
	   _edata = . ;
	} > RamLoc8 AT>MFlash16


    /* MAIN BSS SECTION */
    .bss : ALIGN(4)
    {
        _bss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4) ;
        _ebss = .;
        PROVIDE(end = .);
    } > RamLoc8


    /* DEFAULT NOINIT SECTION */
    .noinit (NOLOAD): ALIGN(4)
    {
        _noinit = .;
        *(.noinit*)
         . = ALIGN(4) ;
        _end_noinit = .;
    } > RamLoc8

    PROVIDE(_HeapStart = .);
    PROVIDE(_MainStackTop = __top_RamLoc - 0);
}
