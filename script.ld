ENTRY(_start)

KERNEL_BASE_LMA = 0x00100000;
KERNEL_BASE_VMA = 0x80000000;
KERNEL_ADDRESS_SPACE_START = KERNEL_BASE_VMA;
KERNEL_ADDRESS_SPACE_END = 0xFFFFFFFF;
USER_ADDRESS_SPACE_START = 0x400000;
USER_ADDRESS_SPACE_END = KERNEL_ADDRESS_SPACE_START - 0x1000;
VMA_MINUS_LMA = KERNEL_BASE_VMA - KERNEL_BASE_LMA;
 
SECTIONS
{
	. = KERNEL_BASE_LMA;
	KERNEL_CODE_BASE = KERNEL_BASE_VMA;
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
	/*TODO*/
	.virt_block_table ALIGN(4K):
	{
		KERNEL_END = .;
		KERNEL_VIRT_BLOCK_TABLE = .;
	}
	. = . & 0xffc00000 + 0x3ff000;
	.temp_page ALIGN(4K):
	{

	}
 
}