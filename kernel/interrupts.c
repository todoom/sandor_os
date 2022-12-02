#include "include/stdlib.h"
#include "include/interrupts.h"
#include "include/memory_manager.h"

#include "include/PIC.h"
#include "include/PIT.h"

IntDesc *idt;
IDTR idtr;

void init_interrupts() 
{
	idt = alloc_virt_pages(NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
	memset(idt, 0, 256 * sizeof(IntDesc));
	IDTR idtr = {256 * sizeof(IntDesc), idt};
	asm("lidt (,%0,)"::"a"(&idtr));

	init_PIC();
	init_PIT();

	set_int_handler(0, 8, default_handler, 0x8E);
	
}

void set_int_handler(uint8_t index, uint16_t sel, void *handler, uint8_t type) 
{
	idt[index].selector = sel;
	idt[index].address_0_15 = (size_t)handler & 0xFFFF;
	idt[index].address_16_31 = (size_t)handler >> 16;
	idt[index].type = type;
	idt[index].reserved = 0;
}

void gen_interrupt(int i)
{
	asm("movl $gen_int, %%ebx \n"
		"movl %%eax, 1(%%ebx) \n"
		"jmp gen_int \n"
		"gen_int:"
		"int $0"::"a"(i));
}

void _default_handler()
{

	printf("%s\n", "default_handler");
}