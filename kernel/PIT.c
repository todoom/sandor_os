#include "include/PIT.h"
#include "include/stdlib.h"

void init_PIT()
{
	outportb(0x43, 0x36);
	outportb(0x40, 0xff);
	outportb(0x40, 0xff);	
	set_int_handler(0, 8, timer_handler, 0x8e);
}
void timer_handler()
{
	(*((char*)(0xB8000)))++;
}