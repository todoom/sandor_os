format ELF
use32

public _start
extrn kernel_main
extrn KERNEL_BASE_LMA
extrn KERNEL_BASE_VMA
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
multiboot_info dd 0
multiboot_header_magic dd 0

_start:
	mov dword[multiboot_info], ebx
	mov dword[multiboot_header_magic], eax

	mov esp, kernel_stack + KERNEL_STACK_SIZE
	mov ebx, esp
	
	; ebx = VMA - LMA
	mov ebx, KERNEL_BASE_VMA
	sub ebx, KERNEL_BASE_LMA
	
	;eax = page_dir - KERNEL_BASE_VMA + KERNEL_BASE_LMA
	;ecx = temp_page_table - KERNEL_BASE_VMA + KERNEL_BASE_LMA
	mov eax, page_dir
	sub eax, ebx
	mov ecx, temp_page_table + 111b
	sub ecx, ebx
	mov dword[eax], ecx
	mov ecx, 1024
	mov edi, temp_page_table
	sub edi, ebx
	mov eax, 11b
@@:
	stosd
	add eax, 0x1000
	loop @b

	;eax = page_dir - KERNEL_BASE_VMA + KERNEL_BASE_LMA + (KERNEL_BASE_VMA >> 22) * 4
	;ecx = kernel_page_table - KERNEL_BASE_VMA + KERNEL_BASE_LMA + 111b
	mov eax, KERNEL_BASE_VMA
	shr eax, 22
	shl eax, 2
	add eax, page_dir
	sub eax, ebx
	mov ecx, kernel_page_table + 111b
	sub ecx, ebx
	mov dword[eax], ecx
	;количество страниц
	mov eax, KERNEL_END
	sub eax, KERNEL_BASE_VMA 
	mov ecx, 0x00001000
	div ecx
	mov ecx, eax
	;смещение
	;edi = kernel_page_table - KERNEL_BASE_VMA + KERNEL_BASE_LMA + KERNEL_BASE_VMA shr 12 and 0x3ff * 4
	mov edi, KERNEL_BASE_VMA
	shr edi, 12
	and edi, 0x3ff
	shl edi, 2
	add edi, kernel_page_table
	sub edi, ebx
	mov eax, KERNEL_BASE_LMA
	add eax, 11b
@@:
	stosd
	add eax, 0x1000
	loop @b

	mov eax, page_dir
	sub eax, ebx
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

section ".text" executable
public flush_gdtr
flush_gdtr:
	pop ebx
	pop eax
	push ebx
	lgdt [eax]
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	jmp 0x8:flush
flush:
	ret	

public flush_tss
flush_tss:
	pop ebx
	pop eax
	push ebx
	ltr [eax]	
	ret

public flush_cr3
flush_cr3:
	pop ebx
	pop eax
	push ebx
	mov cr3, eax
	ret

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
	

