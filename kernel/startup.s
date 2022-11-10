format ELF

public _start
extrn kernel_main
extrn 
extrn KERNEL_CODE_BASE
extrn KERNEL_DATA_BASE
extrn KERNEL_BSS_BASE
extrn KERNEL_END

MAGIC_NUMBER equ 0x1BADB002    
FLAGS        equ 0x0       
CHECKSUM     equ -MAGIC_NUMBER

KERNEL_STACK_SIZE equ 4096

section ".text" executable
dd MAGIC_NUMBER
dd FLAGS
dd CHECKSUM

_start:

	xor ax, ax
	mov cx, 3 * 4096 / 2
	mov di, page_table
	rep stosw

	mov esp, kernel_stack + KERNEL_STACK_SIZE
	call kernel
 @@:
 	hlt
	jmp @b

kernel:

section ".bss"
align 4                                   
kernel_stack:                             
    rb KERNEL_STACK_SIZE 
page_table:
	rb 0x2000
