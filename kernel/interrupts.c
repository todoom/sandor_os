#include "include/stdlib.h"
#include "include/interrupts.h"
#include "include/memory_manager.h"

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

IntDesc *idt;

void timer_int_handler();
void init_PIC();
void init_sys_timer();

void init_interrupts() {
	idt = alloc_virt_pages(NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
	memset(idt, 0, 256 * sizeof(IntDesc));
	IDTR idtr = {256 * sizeof(IntDesc), idt};
	asm("lidt (,%0,)"::"a"(&idtr));
	irq_base = 0x20;
	irq_count = 16;

	init_PIC();
	init_sys_timer();

	//set_int_handler(irq_base, timer_int_handler, 0x8E);
}
void init_PIC()
{
	//Инициализация контроллера прерываний
	outportb(0x20, 0x11);
	outportb(0x21, irq_base);
	outportb(0x21, 4);
	outportb(0x21, 1);
	outportb(0xA0, 0x11);
	outportb(0xA1, irq_base + 8);
	outportb(0xA1, 2);
	outportb(0xA1, 1);
	//Разрешаем прерывания от таймера и клавиатуры
	outportb(0x21, 0xfc);	
}
void init_sys_timer()
{
	//Инициализация системного таймера
	outportb(0x43, 0x34);
	outportb(0x40, 0xff);
	outportb(0x40, 0xff);	
}

void set_int_handler(uint8_t index, void *handler, uint8_t type) {
	asm("pushf \n cli");
	idt[index].selector = 8;
	idt[index].address_0_15 = (size_t)handler & 0xFFFF;
	idt[index].address_16_31 = (size_t)handler >> 16;
	idt[index].type = type;
	idt[index].reserved = 0;
	asm("popf"); 
	asm("sti");
}

// IRQ_HANDLER(timer_int_handler) {
// 	(*((char*)(0xB8000)))++;
// }