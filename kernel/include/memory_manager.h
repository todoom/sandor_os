#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include "stdlib.h"

#define PAGE_SIZE 0x1000
#define PAGE_OFFSET_BITS 12
#define PAGE_OFFSET_MASK 0xFFF
#define PAGE_TABLE_INDEX_BITS 10
#define PAGE_TABLE_INDEX_MASK 0x3FF

#define PHYADDR_BITS 32

#define PAGE_PRESENT		(1 << 0)
#define PAGE_WRITABLE		(1 << 1)
#define PAGE_USER		    (1 << 2)
#define PAGE_WRITE_THROUGH	(1 << 3)
#define PAGE_CACHE_DISABLED	(1 << 4)
#define PAGE_ACCESSED		(1 << 5)

#define PAGE_MODIFIED		(1 << 6)
#define PAGE_GLOBAL		    (1 << 8)

extern uint32_t KERNEL_BASE[];
extern uint32_t KERNEL_CODE_BASE[];
extern uint32_t KERNEL_DATA_BASE[];
extern uint32_t KERNEL_BSS_BASE[];
extern uint32_t KERNEL_END[];

#define KERNEL_PAGE_TABLE 0xFFFFE000 
#define TEMP_PAGE 0xFFFFF000 
#define TEMP_PAGE_INFO (KERNEL_PAGE_TABLE + ((TEMP_PAGE >> PAGE_OFFSET_BITS) & PAGE_TABLE_INDEX_MASK) * sizeof(physaddr))

typedef size_t physaddr;

typedef struct {
	physaddr next;
	physaddr prev;
	size_t size;
} PhysMemoryBlock;

typedef struct {
	uint64_t base;
	uint64_t length;
	uint32_t type;
	uint32_t acpi_ext_attrs;
} __attribute__((packed)) MemoryMapEntry;

/*
typedef enum {
	VMB_RESERVED,
	VMB_MEMORY,
	VMB_IO_MEMORY
} VirtMemoryBlockType;
*/
typedef struct {
	//VirtMemoryBlockType type;
	void *base;
	size_t size;
} VirtMemoryBlock;

typedef struct
{
	size_t block_count;
	VirtMemoryBlock* blocks_table;
	size_t table_size;
} VirtMemory;

typedef struct {
	physaddr page_dir;
	void *start;
	void *end;
	size_t block_table_size;
	size_t block_count;
	VirtMemoryBlock *blocks;
} AddressSpace;

typedef struct
{
	void *base;
	size_t length;
} DynamicMemoryBlock;

typedef struct
{
	size_t block_count;
	DynamicMemoryBlock* blocks;
} DynamicMemory;

extern DynamicMemory dynamic_memory;
extern VirtMemory virt_memory;

void init_memory_manager(void *memory_map) asm ("init_memory_manager");

void temp_map_page(physaddr addr) asm ("temp_map_page");
physaddr get_physaddr(void *vaddr) asm ("get_physaddr");
size_t get_free_memory_size();

bool map_pages(void *vaddr, physaddr paddr, size_t count, unsigned int flags) asm("map_pages");
bool unmap_pages(void *vaddr, size_t count) asm("unmap_pages");

physaddr alloc_phys_pages(size_t count) asm("alloc_phys_pages");
void free_phys_pages(physaddr base, size_t count) asm("free_phys_pages");

extern void *alloc_virt_pages(void *vaddr, physaddr paddr, size_t count, unsigned int flags) asm("alloc_virt_pages");
extern size_t free_virt_pages(void *vaddr) asm("free_virt_pages");

void* add_virt_block(size_t size) asm("add_virt_block");
size_t del_virt_block(void* base) asm("del_virt_block");

extern void* kmalloc(size_t size) asm("kmalloc");
extern void kfree(void* ptr) asm("kfree");
#endif