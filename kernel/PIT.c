#include "include/PIT.h"
#include "include/stdlib.h"

void init_PIT()
{
	outportb(0x43, 0x36);
	outportb(0x40, 0xff);
	outportb(0x40, 0xff);	
}