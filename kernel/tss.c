#include "include/tss.h"
#include "include/gdt.h"

struct tss_entry TSS;

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