all: assembly_os create_iso clean

assembly_os: main.o startup.o stdlib.o tty.o interrupts.o memory_manager.o shell.o ATA.o
	ld -T script.ld -o bin/kernel.bin -shared -ffreestanding -melf_i386 -nostdlib startup.o main.o stdlib.o tty.o interrupts.o memory_manager.o shell.o ATA.o

main.o: kernel/main.cpp
	g++ -c -m32 -ffreestanding -nostdlib -o main.o kernel/main.cpp

startup.o: kernel/startup.s
	gcc -c -m32 -ffreestanding -nostdlib -o startup.o kernel/startup.s

stdlib.o: kernel/stdlib.c
	gcc -c -m32 -ffreestanding -nostdlib -o stdlib.o kernel/stdlib.c 

tty.o: kernel/tty.c
	gcc -c -m32 -ffreestanding -nostdlib -o tty.o kernel/tty.c

interrupts.o: kernel/interrupts.c
	gcc -c -m32 -ffreestanding -nostdlib -o interrupts.o kernel/interrupts.c 

memory_manager.o: kernel/memory_manager.o
	gcc -c -m32 -ffreestanding -nostdlib -o memory_manager.o kernel/memory_manager.c 

shell.o: kernel/shell.c
	gcc -c -m32 -ffreestanding -nostdlib -o shell.o kernel/shell.c  

ATA.o: kernel/ATA.c
	gcc -c -m32 -ffreestanding -nostdlib -o ATA.o kernel/ATA.c  


create_iso:
	mkdir -p isodir/boot/grub
	cp bin/kernel.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o os.iso isodir

clean:
	rm -rf *.o 
	rm -r isodir