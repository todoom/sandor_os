#include "include/stdlib.h"
#include "include/interrupts.h"
#include "include/memory_manager.h"
#include "include/tty.h"
#include "include/shell.h"
#include "include/List.hpp"

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
}