macro align value { db value-1 - ($ + value-1) mod (value) dup 0 }

Begin:
	file "bin\boot.bin" 
	align 512
	file "bin\boot_header.bin" 
	align 512
	file "bin\config.bin"
	align 512
	file "bin\kernel_header.bin"
	align 512
	file "bin\kernel.bin"
	

