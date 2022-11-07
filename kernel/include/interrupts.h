#ifndef INTERRUPTS_H
#define INTERRUPTS_H

uint8_t irq_base;
uint8_t irq_count;

#define IRQ_HANDLER(name) void name(){ \
	asm(#name ": pusha \n \
				 call __" #name " \n \
				 movb $0x20, %al \n \
				 outb %al, $0x20 \n \
				 outb %al, $0xA0 \n \
				 popa \n \
				 iret");} \
	void _ ## name()

void init_interrupts() asm("init_interrupts");
void set_int_handler(uint8_t index, void *handler, uint8_t type);

#endif