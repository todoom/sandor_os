#include "include/tty.h"
#include "include/memory_manager.h"
#include "include/gdt.h"

struct gdt_entry gdt[6];
struct gdt_ptr _gp;

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

void gdt_install()
{
	_gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
	_gp.base = get_physaddr((void*)gdt);

	create_descriptor(0, 0, 0, 0, 0);
									 //1 1 0 0
	create_descriptor(1, 0, 0xFFFFFFFF, GDT_ACCESS_CODE_PL0, 0xC); // 1 00 1 1 0 1 0
	create_descriptor(2, 0, 0xFFFFFFFF, GDT_ACCESS_DATA_PL0, 0xC); // 1 00 1 0 0 1 0

	create_descriptor(3, 0, 0xFFFFFFFF, GDT_ACCESS_CODE_PL3, 0xC); // 1 11 1 1 0 1 0
	create_descriptor(4, 0, 0xFFFFFFFF, GDT_ACCESS_DATA_PL3, 0xC); // 1 11 1 0 0 1 0

	set_gdtr(get_physaddr((void*)(&_gp)));
}