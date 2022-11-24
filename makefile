all: assembly_os create_iso run

assembly_os: main.o startup.o stdlib.o tty.o interrupts.o memory_manager.o shell.o ATA.o gdt.o tss.o
	i386-elf-ld -T script.ld -o bin/kernel.elf startup.o main.o stdlib.o tty.o interrupts.o memory_manager.o shell.o ATA.o gdt.o tss.o
	rm -rf *.o 

main.o: kernel/main.cpp
	i386-elf-g++ -c -m32 -ffreestanding -nostdlib -o main.o kernel/main.cpp

startup.o: kernel/startup.s
	fasm kernel/startup.s startup.o 

stdlib.o: kernel/stdlib.c
	i386-elf-gcc -c -m32 -ffreestanding -nostdlib -o stdlib.o kernel/stdlib.c 

tty.o: kernel/tty.c
	i386-elf-gcc -c -m32 -ffreestanding -nostdlib -o tty.o kernel/tty.c

interrupts.o: kernel/interrupts.c
	i386-elf-gcc -c -m32 -ffreestanding -nostdlib -o interrupts.o kernel/interrupts.c 

memory_manager.o: kernel/memory_manager.c
	i386-elf-gcc -c -m32 -ffreestanding -nostdlib -o memory_manager.o kernel/memory_manager.c 

shell.o: kernel/shell.c
	i386-elf-gcc -c -m32 -ffreestanding -nostdlib -o shell.o kernel/shell.c  

ATA.o: kernel/ATA.c
	i386-elf-gcc -c -m32 -ffreestanding -nostdlib -o ATA.o kernel/ATA.c  

gdt.o: kernel/gdt.c
	i386-elf-gcc -c -m32 -ffreestanding -nostdlib -o gdt.o kernel/gdt.c  

tss.o: kernel/tss.c
	i386-elf-gcc -c -m32 -ffreestanding -nostdlib -o tss.o kernel/tss.c  

create_iso:
	mkdir -p isodir/boot/grub
	cp bin/kernel.elf isodir/boot/kernel.elf
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o os.iso isodir

run: os.iso
	bochs -f bochs.cfg -q