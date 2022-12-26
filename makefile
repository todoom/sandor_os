all: assembly_os create_iso run

assembly_os: main.o startup.o stdlib.o tty.o interrupts.o interrupts2.o memory_manager.o shell.o ATA.o gdt.o PIC.o PIT.o task.o
	i386-elf-ld -T script.ld -o bin/kernel.elf startup.o main.o stdlib.o tty.o interrupts.o interrupts2.o memory_manager.o shell.o ATA.o gdt.o PIT.o PIC.o task.o
	rm -rf *.o 

main.o: kernel/main.cpp
	i386-elf-g++ -c -m32 -ffreestanding -nostdlib -o main.o kernel/main.cpp

startup.o: kernel/startup.asm
	fasm kernel/startup.asm startup.o 

stdlib.o: kernel/stdlib.c
	i386-elf-gcc -c -m32 -ffreestanding -nostdlib -o stdlib.o kernel/stdlib.c 

tty.o: kernel/tty.c
	i386-elf-gcc -c -m32 -ffreestanding -nostdlib -o tty.o kernel/tty.c

interrupts.o: kernel/interrupts.c
	i386-elf-gcc -c -m32 -ffreestanding -nostdlib -o interrupts.o kernel/interrupts.c 
	
interrupts2.o: kernel/interrupts.asm
	fasm kernel/interrupts.asm interrupts2.o

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

tss2.o: kernel/tss.asm
	fasm kernel/tss.asm tss2.o  

PIC.o: kernel/PIC.c
	i386-elf-gcc -c -m32 -ffreestanding -nostdlib -o PIC.o kernel/PIC.c

PIT.o: kernel/PIT.c
	i386-elf-gcc -c -m32 -ffreestanding -nostdlib -o PIT.o kernel/PIT.c  

task.o: kernel/task.c
	i386-elf-gcc -c -m32 -ffreestanding -nostdlib -o task.o kernel/task.c  	

create_iso:
	mkdir -p isodir/boot/grub
	cp bin/kernel.elf isodir/boot/kernel.elf
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o os.iso isodir

run: os.iso
	bochs -f bochs.cfg -q