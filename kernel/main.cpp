#include "include/stdlib.h"
#include "include/interrupts.h"
#include "include/memory_manager.h"
#include "include/tty.h"
#include "include/shell.h"
#include "include/List.hpp"
#include "include/ATA.h"
#include "include/multiboot.h"

extern void kernel_main(char boot_disk_id, void *memory_map, BootModuleInfo *boot_module_list) asm("kernel_main");

void kernel_main(multiboot_info_t* mbd, uint32_t magic) 
{
	
	init_memory_manager(mbd->mmap_addr);
	init_interrupts();
	init_tty();
	clear_screen();

	printf("%x\n", 12345);
}