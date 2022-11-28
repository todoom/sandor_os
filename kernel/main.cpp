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

	DynamicMemory *current_dynamic_memory = &(current_address_space->dynamic_memory);
	
	printf("%x\n", current_dynamic_memory->blocks);
	
	for (int i = 0; i < 0x201; i++)
	{
		kmalloc(1);
	}
	// size_t size = current_dynamic_memory->block_count;
	// for (int i = 0; i < size - 1; i++)
	// {
	// 	printf("%x\n", kfree(current_dynamic_memory->blocks[0].base));
	// }
	for (int i = 0; i < current_dynamic_memory->block_count; i++)
	{
		printf("%x\n", current_dynamic_memory->blocks[i].base);
		printf("%x\n", current_dynamic_memory->blocks[i].size);
	}
	printf("%x\n", current_dynamic_memory->blocks);
	printf("%s\n", "complete");
}