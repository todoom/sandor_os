format ELF

public _start
extrn kernel_main
extrn _KERNEL_BASE
extrn _KERNEL_CODE_BASE
extrn _KERNEL_DATA_BASE
extrn _KERNEL_BSS_BASE
extrn _KERNEL_END

MAGIC_NUMBER equ 0x1BADB002    
FLAGS        equ 0x0       
CHECKSUM     equ -MAGIC_NUMBER

KERNEL_STACK_SIZE equ 4096

section ".text" executable
dd MAGIC_NUMBER
dd FLAGS
dd CHECKSUM

_start:
	xor eax, eax
	mov ecx, 3 * 4096 / 2
	mov edi, page_table
	rep stosw

	mov dword[page_table], page_table + 0x1000 + 111b
	mov ecx, 256
	mov eax, 0x100000 + 11b
	mov edi, page_table + 0x1400
@@:
	stosd
	add eax, 0x1000
	loop @b

	mov eax, page_table
	mov cr3, eax

	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

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

section ".bss" align 0x1000
page_table:
	rb 0x2000

section ".bss"
trash:
	rb 0x1