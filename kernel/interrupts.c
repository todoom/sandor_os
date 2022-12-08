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

	for (int i = 1; i < 256; i++)
	{
		set_int_handler(i, 8, default_handler, 0x8E);
	}	

	init_PIC();
	init_PIT();
}

void set_int_handler(uint8_t index, uint16_t sel, void *handler, uint8_t type) 
{
	idt[index].selector = sel;
	idt[index].type = type;
	idt[index].reserved = 0;

	void *temp = kmalloc((size_t)wrapper_size);
	memcpy(temp, handler_wrapper_template, wrapper_size);

	int new_call_addr = (size_t)handler - ((size_t)temp + call_handler_offset + call_size);
	memcpy(temp + call_handler_offset + 1, &new_call_addr, sizeof(size_t));

	idt[index].address_0_15 = (size_t)temp & 0xFFFF;
	idt[index].address_16_31 = (size_t)temp >> 16;
}

void gen_interrupt(int i)
{
	asm("movl $gen_int, %%ebx \n"
		"movb %%al, 1(%%ebx) \n"
		"jmp gen_int \n"
		"gen_int:"
		"int $0"::"a"(i));
}

void default_handler()
{
	printf("%s\n", "default_handler");
}