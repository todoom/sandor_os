#include "include/task.h"


void enter_usermode()
{
	asm("cli \n"
		"movl $0x23, %eax \n"
		"movl %eax, %ds \n"
		"movl %eax, %es \n"
		"movl %eax, %fs \n"
		"movl %eax, %gs \n"
		"push $0x23 \n"
		"push %esp \n"
		"pushfl \n"
		"pop %eax \n"
		"or $0x200, %eax \n"
		"push %eax \n"
		"push $0x1b \n"
		"movl $a, %eax \n"
		"push %eax \n"
		"iretl \n"
		"a: addl $4, %esp");
}
