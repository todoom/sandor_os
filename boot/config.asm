f_info:
	f_name db "boot.cfg" ; 5
	times 256 - ($ - $$) db 0
	f_next dq 8
	f_prev dq 3
	f_parent dq -1
	f_flags dq 1
	f_data dq 6
	f_size dq 0
	f_ctime dq 0
	f_mtime dq 0
	f_atime dq 0

times 512 - ($ - $$) db 0

; Вторые 512 байт список сектров с данными файла
dw 7, 0, -1, -1
times 512 - ($ - 512) db 0

db "Lkernel.bin", 0
db "S32", 0