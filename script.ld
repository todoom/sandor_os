ENTRY(_start)
 
SECTIONS
{
	. = 0x100000;
 	_KERNEL_BASE = .;
	.text BLOCK(4K) : ALIGN(4K)
	{
		_KERNEL_CODE_BASE = .;
		*(.text)
	}
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		_KERNEL_DATA_BASE = .;
		*(.rodata)
	}
 
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}
 
	.bss BLOCK(4K) : ALIGN(4K)
	{
		_KERNEL_BSS_BASE = .;
		*(COMMON)
		*(.bss)
	}
	.empty ALIGN(0x1000) - 1 : {
		BYTE(0)
		_KERNEL_END = .;
	}
 
}