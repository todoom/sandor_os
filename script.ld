ENTRY(_start)
 
SECTIONS
{
	. = 0x100000;
	KERNEL_BASE = .;
 	.multiboot.data : 
 	{
        *(.multiboot.data)
    }
   	.multiboot.text : 
    {
        *(.multiboot.text)
    }

    . += 0xC0000000;
	.text : AT (ADDR(.text) - 0xC0000000)
	{
		KERNEL_CODE_BASE = .;
		*(.text)
	}
	.rodata ALIGN(4K) : AT (ADDR(.rodata) - 0xC0000000)
	{
		KERNEL_DATA_BASE = .;
		*(.rodata)
	}
 
	.data ALIGN(4K) : AT (ADDR(.data) - 0xC0000000)
	{
		*(.data)
	}
 
	.bss ALIGN(4K) : AT (ADDR(.bss) - 0xC0000000)
	{
		KERNEL_BSS_BASE = .;
		*(COMMON)
		*(.bss)
	}
	.page_dir ALIGN(4K) : AT (ADDR(.page_dir) - 0xC0000000)
	{
		*(.page_dir)
		KERNEL_PAGE_TABLE = .;
		*(.kernel_page_table);
	}
	.empty ALIGN(0x1000) - 1 : AT (ADDR(.empty) - 0xC0000000)
	{
		BYTE(0)
		KERNEL_END = .;
	}
 
}