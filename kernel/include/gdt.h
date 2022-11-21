#ifndef GDT_H
#define GDT_H

#include "stdlib.h"

extern void set_gdtr(size_t addr);

struct gdt_entry
{
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_middle;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr
{
	unsigned short limit;
	unsigned int base;
} __attribute__((packed));



extern void gdt_install() asm("gdt_install");

#endif 