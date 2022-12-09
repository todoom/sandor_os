format ELF
use32

section '.text' executable

public handler_wrapper_template
public call_handler_offset
public call_size
public wrapper_size

handler_wrapper_template:
	pushad

start_call:
	call dword start_call
end_call:
	
	mov al, 0x20
	out 0x20, al
	out 0xa0, al
	
	popad
	iretd 


wrapper_size dd $ - handler_wrapper_template
call_handler_offset dd start_call - handler_wrapper_template
call_size dd end_call - start_call