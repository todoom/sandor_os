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

#define USER_MEMORY_START ((void*)0)
#define USER_MEMORY_END ((void*)0x7FFFFFFF)
#define KERNEL_MEMORY_START ((void*)0x80000000)
#define KERNEL_MEMORY_END ((void*)(KERNEL_BASE))

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

typedef enum {
	VMB_RESERVED,
	VMB_MEMORY,
	VMB_IO_MEMORY
} VirtMemoryBlockType;

typedef struct {
	VirtMemoryBlockType type;
	void *base;
	size_t length;
} VirtMemoryBlock;

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


extern physaddr kernel_page_dir;
extern size_t memory_size;
extern AddressSpace kernel_address_space;
extern DynamicMemory dynamic_memory;

extern void init_memory_manager(void *memory_map) asm ("init_memory_manager");

void temp_map_page(physaddr addr);
bool map_pages(physaddr page_dir, void *vaddr, physaddr paddr, size_t count, unsigned int flags);
physaddr get_physaddr(void *vaddr);

size_t get_free_memory_size();
physaddr alloc_phys_pages(size_t count);
void free_phys_pages(physaddr base, size_t count);

void create_memory_block(void* memory_map, physaddr paddr, size_t size, size_t type);
void cut_memory_block(void* memory_map, physaddr memory_block_paddr, size_t cut_size);

void *alloc_virt_pages(AddressSpace *address_space, void *vaddr, physaddr paddr, size_t count, unsigned int flags);
bool free_virt_pages(AddressSpace *address_space, void *vaddr, unsigned int flags);

extern void* kmalloc(size_t size) asm("kmalloc");
extern void kfree(void* ptr) asm("kfree");
#endif