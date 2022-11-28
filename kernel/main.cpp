#include "include/stdlib.h"
#include "include/interrupts.h"
#include "include/memory_manager.h"
#include "include/tty.h"
#include "include/shell.h"
#include "include/List.hpp"
#include "include/ATA.h"
#include "include/multiboot.h"
#include "include/gdt.h"

extern void kernel_main(multiboot_info_t* mbd, uint32_t magic) asm("kernel_main");

void kernel_main(multiboot_info_t* mbd, uint32_t magic) 
{
	init_memory_manager(mbd->mmap_addr);
	init_interrupts();
	init_tty();
	clear_screen();

	// for (int i = 0; i < 200000; i++) {alloc_virt_pages(NULL, 0xFFFFF000, 1, PAGE_PRESENT);}

	printf("%s\n", "ok");
	printf("%s\n", "hello world");
	printf("%x\n", 0x12345);
	void *temp;
	for (int i = 0; i < 0x1000; i++)
	{
		temp = alloc_virt_pages(NULL, -1, 1, PAGE_PRESENT);
	}
	for (int i = 0; i < 1024; i++, temp -= 0x1000)
	{
		free_virt_pages(temp);
	}
	printf("%s\n", "complete");
}