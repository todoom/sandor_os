#include "include/stdlib.h"
#include "include/interrupts.h"
#include "include/memory_manager.h"
#include "include/tty.h"
#include "include/shell.h"
#include "include/List.hpp"
#include "include/ATA.h"

typedef struct 
{
	unsigned long long base;
	unsigned long long size;
} BootModuleInfo;

extern void kernel_main(char boot_disk_id, void *memory_map, BootModuleInfo *boot_module_list) asm ("kernel_main");

void kernel_main(char boot_disk_id, void *memory_map, BootModuleInfo *boot_module_list) 
{
	init_memory_manager(memory_map);
	init_interrupts();
	init_tty();
	clear_screen();

	init_ATA_devices();

	void *buffer = kmalloc(0x200);
	read_ATA(ata_devices[0], 1, 1, buffer);
	printf("%x\n", get_physaddr(buffer));
}