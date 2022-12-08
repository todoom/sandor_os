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

extern void handler_wrapper_template() asm("handler_wrapper_template");
extern size_t wrapper_size;
extern size_t call_handler_offset;
extern size_t call_size;

void default_handler();
void gen_interrupt(int i) asm("gen_interrupt");
void init_interrupts() asm("init_interrupts");
void set_int_handler(uint8_t index, uint16_t sel, void *handler, uint8_t type) asm("set_int_handler");

#endif