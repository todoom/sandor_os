ENTRY(_start)

KERNEL_BASE_LMA = 0x00100000;
KERNEL_BASE_VMA = 0xbbbbb000;
VMA_MINUS_LMA = KERNEL_BASE_VMA - KERNEL_BASE_LMA;
 
SECTIONS
{
	. = KERNEL_BASE_LMA;
 	.multiboot.data : 
 	{
        *(.multiboot.data)
    }
   	.multiboot.text : 
    {
        *(.multiboot.text)
    }

    . = . + VMA_MINUS_LMA;
	.text : AT (ADDR(.text) - VMA_MINUS_LMA)
	{
		KERNEL_CODE_BASE = .;
		*(.text)
	}
	.rodata ALIGN(4K) : AT (ADDR(.rodata) - VMA_MINUS_LMA)
	{
		KERNEL_DATA_BASE = .;
		*(.rodata)
	}
 
	.data ALIGN(4K) : AT (ADDR(.data) - VMA_MINUS_LMA)
	{
		*(.data)
	}
 
	.bss ALIGN(4K) : AT (ADDR(.bss) - VMA_MINUS_LMA)
	{
		KERNEL_BSS_BASE = .;
		*(COMMON)
		*(.bss)
	}
	.page_dir ALIGN(4K) : AT (ADDR(.page_dir) - VMA_MINUS_LMA)
	{
		*(.page_dir)
		KERNEL_PAGE_TABLE = .;
		*(.kernel_page_table);
	}
	.empty ALIGN(0x1000) - 1 : AT (ADDR(.empty) - VMA_MINUS_LMA)
	{
		BYTE(0)
		KERNEL_END = .;
	}
 
}