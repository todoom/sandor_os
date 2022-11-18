format ELF
use32

public _start
extrn kernel_main
extrn KERNEL_END
KERNEL_BASE_VMA equ 0xc0000000
KERNEL_BASE_LMA equ 0x00100000
VMA_MINUS_LMA equ (KERNEL_BASE_VMA - KERNEL_BASE_LMA)

MAGIC_NUMBER equ 0x1BADB002    
FLAGS        equ 0x0       
CHECKSUM     equ -MAGIC_NUMBER

KERNEL_STACK_SIZE equ 4096

section ".multiboot.data"
align 4
dd MAGIC_NUMBER
dd FLAGS
dd CHECKSUM

section ".multiboot.text" executable
multiboot_info dd 0
multiboot_header_magic dd 0

_start:
	mov dword[multiboot_info], ebx
	mov dword[multiboot_header_magic], eax

	mov esp, kernel_stack + KERNEL_STACK_SIZE
	mov ebx, esp
	
	mov dword[page_dir - VMA_MINUS_LMA], temp_page_table - VMA_MINUS_LMA + 111b
	mov ecx, 1024
	mov edi, temp_page_table - VMA_MINUS_LMA
	mov eax, 11b
@@:
	stosd
	add eax, 0x1000
	loop @b

	mov dword[page_dir - VMA_MINUS_LMA + (KERNEL_BASE_VMA shr 22) * 4], kernel_page_table - VMA_MINUS_LMA + 111b
	;количество страниц
	mov eax, KERNEL_END
	sub eax, KERNEL_BASE_VMA 
	mov ebx, 0x00001000
	div ebx
	mov ecx, eax
	;смещение
	mov edi, kernel_page_table - VMA_MINUS_LMA + KERNEL_BASE_VMA shr 12 and 0x3ff * 4
	mov eax, KERNEL_BASE_LMA + 11b
@@:
	stosd
	add eax, 0x1000
	loop @b

	mov eax, page_dir - VMA_MINUS_LMA
	mov cr3, eax

	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

	push [multiboot_header_magic]
	push [multiboot_info]
	
	call kernel_main
@@:
 	hlt
	jmp @b

section ".bss"
align 4                                   
kernel_stack:                             
    rb KERNEL_STACK_SIZE 

section ".page_dir" align 0x1000
page_dir:
	rb 0x1000
temp_page_table:
	rb 0x1000

section ".kernel_page_table" align 0x1000
kernel_page_table:
	rb 0xfff

