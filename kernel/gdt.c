#include "include/tty.h"
#include "include/memory_manager.h"
#include "include/gdt.h"

struct gdt_entry gdt[6];
struct gdt_ptr _gp;
struct tss_entry TSS;

void create_descriptor(uint16_t num, uint32_t base, \
				 uint32_t limit, uint8_t access, uint8_t flags)
{
	gdt[num].limit_low = limit & 0xffff;
	gdt[num].granularity = (limit >> 16) & 0x0f;
	gdt[num].base_low = base & 0xffff;
	gdt[num].base_middle = (base >> 16) & 0xff;
	gdt[num].access = access;
	gdt[num].granularity |= (flags << 4);
}

void flush_gdtr(physaddr gdtr)
{
	asm("lgdt (,%0,) \n"
		"movw $0x10, %%ax \n"
		"movw %%ax, %%ds \n"
		"movw %%ax, %%es \n"
		"movw %%ax, %%fs \n"
		"movw %%ax, %%gs \n"
		"movw %%ax, %%ss \n"
		"ljmp $0x8, $flush \n"
		"flush: \n"
		::"a"(gdtr));
}

void flush_tss(uint32_t sel)
{
	asm("ltr (,%0,) \n"::"r"(sel));
}

void tss_install(uint32_t idx, uint16_t kernelSS, uint16_t kernelESP)
{
	uint32_t base = (uint32_t)&TSS;

	create_descriptor(idx, get_physaddr(base), get_physaddr(sizeof(struct tss_entry)), 0x89, 0x0);

	memset ((void*) &TSS, 0, sizeof (struct tss_entry));

	TSS.ss0 = kernelSS;
	TSS.esp0 = kernelESP;

	TSS.cs = 0x0b;
	TSS.ss = 0x13;
	TSS.es = 0x13;
	TSS.ds = 0x13;
	TSS.fs = 0x13;
	TSS.gs = 0x13;
}

void gdt_install()
{
	_gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
	_gp.base = get_physaddr((void*)gdt);

	create_descriptor(0, 0, 0, 0, 0);
									 
	create_descriptor(1, 0, 0xFFFFFFFF, GDT_ACCESS_CODE_PL0, GDT_FLAGS); 
	create_descriptor(2, 0, 0xFFFFFFFF, GDT_ACCESS_DATA_PL0, GDT_FLAGS); 

	create_descriptor(3, 0, 0xFFFFFFFF, GDT_ACCESS_CODE_PL3, GDT_FLAGS); 
	create_descriptor(4, 0, 0xFFFFFFFF, GDT_ACCESS_DATA_PL3, GDT_FLAGS); 
	tss_install(5, 0x10, 0);

	flush_gdtr(get_physaddr((void*)(&_gp)));
	flush_tss(0x2b);	
}