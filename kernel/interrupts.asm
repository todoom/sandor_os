format ELF
use32

section ".text" executable

public gen_intrrupt2
gen_intrrupt2:
	pop ebx
	pop eax
	push ebx
	mov ebx, .do_int
	mov [ebx + 1], eax
	jmp .do_int
.do_int:
	int 0

public default_handler
extrn _default_handler
default_handler:
	pusha
	
	call _default_handler

	popa
	iret