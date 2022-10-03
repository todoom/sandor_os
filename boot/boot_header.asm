; Первые 512 байт заголовок файла
f_info:
	f_name db "boot.bin" ;3
	times 256 - ($ - $$) db 0
	f_next dq 5
	f_prev dq -1
	f_parent dq -1
	f_flags dq 1
	f_data dq 4
	f_size dq 0
	f_ctime dq 0
	f_mtime dq 0
	f_atime dq 0

times 512 - ($ - $$) db 0

; Вторые 512 байт список сектров с данными файла
dw 1, 0, 2, 0, -1, -1
times 512 - ($ - 512) db 0






