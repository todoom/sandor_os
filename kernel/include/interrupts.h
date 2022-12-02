#ifndef INTERRUPTS_H
#define INTERRUPTS_H

typedef struct {
	uint16_t address_0_15;
	uint16_t selector;
	uint8_t reserved;
	uint8_t type;
	uint16_t address_16_31;
} __attribute__((packed)) IntDesc;

typedef struct {
	uint16_t limit;
	void *base;
} __attribute__((packed)) IDTR;

// #define IRQ_HANDLER(name) void name(){ \
// 	asm(#name ": pusha \n \
// 				 call __" #name " \n \
// 				 movb $0x20, %al \n \
// 				 outb %al, $0x20 \n \
// 				 outb %al, $0xA0 \n \
// 				 popa \n \
// 				 iret");} \
// 	void _ ## name()

extern void default_handler();


void gen_interrupt(int i ) asm("gen_interrupt");

void init_interrupts() asm("init_interrupts");
void set_int_handler(uint8_t index, uint16_t sel, void *handler, uint8_t type) asm("set_int_handler");
void _default_handler() asm("_default_handler");

#endif