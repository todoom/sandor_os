format ELF
use32

public _start
extrn kernel_main
extrn KERNEL_BASE
extrn KERNEL_CODE_BASE
extrn KERNEL_DATA_BASE
extrn KERNEL_BSS_BASE
extrn KERNEL_END

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

_start:
	mov esp, kernel_stack - 0xc0000000 + KERNEL_STACK_SIZE

	mov dword[page_table - 0xc0000000 + 0xc00], kernel_page_table - 0xc0000000 + 111b
	;количество страниц
	mov eax, KERNEL_END
	sub eax, KERNEL_BASE 
	mov ebx, 0x00001000
	div ebx
	mov ecx, eax
	;смещение
	mov eax, KERNEL_BASE
	shr eax, 12
	and eax, 0x3ff
	shl eax, 2
	mov edi, kernel_page_table - 0xc0000000
	add edi, eax
	mov eax, KERNEL_BASE - 0xc0000000 + 11b
@@:
	stosd
	add eax, 0x1000
	loop @b

	mov eax, page_table - 0xc0000000
	mov cr3, eax

	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

	call kernel_main
@@:
 	hlt
	jmp @b

section ".bss"
align 4                                   
kernel_stack:                             
    rb KERNEL_STACK_SIZE 

section ".page_table" align 0x1000
page_table:
	rb 0x1000

section ".kernel_page_table" align 0x1000
kernel_page_table:
	rb 0x999

; section ".page_table" 
; align 4
; trash:
; 	rb 0x1
