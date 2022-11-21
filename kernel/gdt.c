#include "include/tty.h"
#include "include/memory_manager.h"
#include "include/gdt.h"

struct gdt_entry gdt[6];
struct gdt_ptr _gp;

void gdt_set_gate(uint16_t num, uint32_t base, \
				 uint32_t limit, uint8_t access, uint8_t flags)
{
	gdt[num].limit_low = limit & 0xffff;
	gdt[num].granularity = (limit >> 16) & 0x0f;
	gdt[num].base_low = base & 0xffff;
	gdt[num].base_middle = (base >> 16) & 0xff;
	gdt[num].access = access;
	gdt[num].granularity |= (flags << 4);
}

void gdt_install()
{
	_gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
	_gp.base = get_physaddr((void*)gdt);

	gdt_set_gate(0, 0, 0, 0, 0);
									 //1 1 0 0
	gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xC); // 1 00 1 1 0 1 0
	gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xC); // 1 00 1 0 0 1 0

	gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xC); // 1 11 1 1 0 1 0
	gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xC); // 1 11 1 0 0 1 0

	//Возможно что то с это V хуйней
	set_gdtr(get_physaddr(&_gp));

}