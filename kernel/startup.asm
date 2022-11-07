format ELF

public _start
extrn kernel_main

section ".text" executable

_start:
	movzx edx, dl
	push ebx
	push esi
	push edx
	lgdt [gdtr]
	call kernel_main
    add esp, 3 * 4
 @@:
 	hlt
	jmp @b
gdt:
	dq 0                 
	dq 0x00CF9A000000FFFF
	dq 0x00CF92000000FFFF
gdtr:
	dw $ - gdt
	dd gdt 