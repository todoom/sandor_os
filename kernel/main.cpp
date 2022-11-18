#include "include/stdlib.h"
#include "include/interrupts.h"
#include "include/memory_manager.h"
#include "include/tty.h"
#include "include/shell.h"
#include "include/List.hpp"
#include "include/ATA.h"
#include "include/multiboot.h"

extern void kernel_main(multiboot_info_t* mbd, uint32_t magic) asm("kernel_main");

void kernel_main(multiboot_info_t* mbd, uint32_t magic) 
{
	init_memory_manager(mbd->mmap_addr);
	init_interrupts();
	init_tty();
	clear_screen();

	char *str = "Hello, wer\0";
	printf("%s\n", str);
	List<int> list;
	list.insert(5);
	list.insert(4);
	list.insert(3);

	for(int i = 0; i < list.length; i++) printf("%x\n", list.get_value(i));

	// for(PhysMemoryBlock* i = (PhysMemoryBlock*)free_phys_memory_pointer; (physaddr)i != free_phys_memory_pointer; i = (PhysMemoryBlock*)i->next)
	// {
	// 	printf("%x ", i);
	// 	printf("%x ", i->size);
	// 	printf("\n");
	// }
}