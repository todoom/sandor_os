#ifndef GDT_H
#define GDT_H

#include "stdlib.h"
#include "memory_manager.h"

#define SEG_SAVL(x)      ((x) << 0x0) // Available for system use
#define SEG_LONG(x)      ((x) << 0x1) // Long mode
#define SEG_SIZE(x)      ((x) << 0x2) // Size (0 for 16-bit, 1 for 32)
#define SEG_GRAN(x)      ((x) << 0x3) // Granularity (0 for 1B - 1MB, 1 for 4KB - 4GB)

#define SEG_DESCTYPE(x)  ((x) << 0x04) // Descriptor type (0 for system, 1 for code/data)
#define SEG_PRIV(x)     (((x) &  0x03) << 0x05)   // Set privilege level (0 - 3)
#define SEG_PRES(x)      ((x) << 0x07) // Present

#define SEG_DATA_RD        0x00 // Read-Only
#define SEG_DATA_RDA       0x01 // Read-Only, accessed
#define SEG_DATA_RDWR      0x02 // Read/Write
#define SEG_DATA_RDWRA     0x03 // Read/Write, accessed
#define SEG_DATA_RDEXPD    0x04 // Read-Only, expand-down
#define SEG_DATA_RDEXPDA   0x05 // Read-Only, expand-down, accessed
#define SEG_DATA_RDWREXPD  0x06 // Read/Write, expand-down
#define SEG_DATA_RDWREXPDA 0x07 // Read/Write, expand-down, accessed
#define SEG_CODE_EX        0x08 // Execute-Only
#define SEG_CODE_EXA       0x09 // Execute-Only, accessed
#define SEG_CODE_EXRD      0x0A // Execute/Read
#define SEG_CODE_EXRDA     0x0B // Execute/Read, accessed
#define SEG_CODE_EXC       0x0C // Execute-Only, conforming
#define SEG_CODE_EXCA      0x0D // Execute-Only, conforming, accessed
#define SEG_CODE_EXRDC     0x0E // Execute/Read, conforming
#define SEG_CODE_EXRDCA    0x0F // Execute/Read, conforming, accessed

#define GDT_FLAGS SEG_GRAN(1) | SEG_SIZE(1) | SEG_LONG(0) | SEG_SAVL(0)

#define GDT_ACCESS_CODE_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_PRIV(0) | SEG_CODE_EXRD

#define GDT_ACCESS_DATA_PL0 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_PRIV(0) | SEG_DATA_RDWR

#define GDT_ACCESS_CODE_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_PRIV(3) | SEG_CODE_EXRD

#define GDT_ACCESS_DATA_PL3 SEG_DESCTYPE(1) | SEG_PRES(1) | SEG_PRIV(3) | SEG_DATA_RDWR

void flush_gdtr(physaddr gdtr);
extern void create_descriptor(uint16_t num, uint32_t base, \
				 uint32_t limit, uint8_t access, uint8_t flags) asm("create_descriptor");

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
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

extern void gdt_install() asm("gdt_install");

void flush_tss(uint32_t sel);
void tss_install(uint32_t idx, uint16_t kernelSS, uint16_t kernelESP);

struct tss_entry {
	uint32_t prevTss;
	uint32_t esp0;
	uint32_t ss0;
	uint32_t esp1;	
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t trap;
	uint16_t iomap;
};

#endif 