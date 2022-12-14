#include "include/PIT.h"
#include "include/PIC.h"
#include "include/stdlib.h"

void init_PIT()
{
	outportb(0x43, 0x34);
	outportb(0x40, 0xff);
	outportb(0x40, 0xff);	
	set_int_handler(IRQ_0, 8, timer_handler, 0x8e);
}

void enable_PIT()
{
	int i;
	inportb(PIC_1_DATA, i);
	outportb(PIC_1_DATA, i & ~1);
}

void timer_handler()
{
	(*((char*)(0xB8000)))++;
}