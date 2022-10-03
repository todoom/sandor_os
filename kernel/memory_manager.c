#include "include/stdlib.h"
#include "include/memory_manager.h"

size_t free_page_count;
physaddr free_phys_memory_pointer;
physaddr kernel_page_dir;
size_t memory_size;
AddressSpace kernel_address_space;
DynamicMemory dynamic_memory;

void init_memory_manager(void* memory_map) 
{
	//init physical memory
	asm("movl %%cr3, %0":"=a"(kernel_page_dir));
	memory_size = 0x100000;
	free_page_count = 0;
	free_phys_memory_pointer = -1;
	MemoryMapEntry *entry;
	for (entry = memory_map; entry->type; entry++) 
	{
		if ((entry->type == 1) && (entry->base >= 0x100000)) 
		{
			free_phys_pages(entry->base, entry->length >> PAGE_OFFSET_BITS);
			memory_size += entry->length;
		}
	}
	//init virtual memory
	map_pages(kernel_page_dir, (void*)KERNEL_CODE_BASE, get_physaddr((void*)KERNEL_CODE_BASE),
		((size_t)KERNEL_DATA_BASE - (size_t)KERNEL_CODE_BASE) >> PAGE_OFFSET_BITS, PAGE_PRESENT | PAGE_GLOBAL);
	map_pages(kernel_page_dir, \
			  (void*)KERNEL_DATA_BASE, \
			  get_physaddr((void*)KERNEL_DATA_BASE), \
			  ((size_t)KERNEL_END - (size_t)KERNEL_DATA_BASE) >> PAGE_OFFSET_BITS, \
			  PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
	map_pages(kernel_page_dir, (void*)KERNEL_PAGE_TABLE, get_physaddr((void*)KERNEL_PAGE_TABLE), 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);

	kernel_address_space.page_dir = kernel_page_dir;
	kernel_address_space.start = KERNEL_MEMORY_START;
	kernel_address_space.end = KERNEL_MEMORY_END;
	kernel_address_space.block_table_size = PAGE_SIZE / sizeof(VirtMemoryBlock);
	kernel_address_space.blocks = KERNEL_MEMORY_START;
	kernel_address_space.block_count = 1;

	map_pages(kernel_page_dir, kernel_address_space.blocks, alloc_phys_pages(1), 1, PAGE_PRESENT | PAGE_WRITABLE);
	kernel_address_space.blocks[0].type = VMB_RESERVED;
	kernel_address_space.blocks[0].base = kernel_address_space.blocks;
	kernel_address_space.blocks[0].length = PAGE_SIZE;
	dynamic_memory.block_count = 0;
	dynamic_memory.blocks = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
}

static inline void flush_page_cache(void *addr) 
{
	asm("invlpg (,%0,)"::"a"(addr));
}

void temp_map_page(physaddr page) 
{
	*((physaddr*)TEMP_PAGE_INFO) = (page & ~PAGE_OFFSET_MASK) | PAGE_PRESENT | PAGE_WRITABLE;
	flush_page_cache((void*)TEMP_PAGE);
}

void free_phys_pages(physaddr base, size_t count)
{
	if (free_phys_memory_pointer == -1) 
	{
		temp_map_page(base);
		((PhysMemoryBlock*)TEMP_PAGE)->next = base;
		((PhysMemoryBlock*)TEMP_PAGE)->prev = base;
		((PhysMemoryBlock*)TEMP_PAGE)->size = count;
		free_phys_memory_pointer = base;
	} 
	else 
	{
		physaddr cur_block = free_phys_memory_pointer;
		do {
			temp_map_page(cur_block);
			if (cur_block + (((PhysMemoryBlock*)TEMP_PAGE)->size << PAGE_OFFSET_BITS) == base) 
			{
				((PhysMemoryBlock*)TEMP_PAGE)->size += count;
				if (((PhysMemoryBlock*)TEMP_PAGE)->next == base + (count << PAGE_OFFSET_BITS)) 
				{
					physaddr next1 = ((PhysMemoryBlock*)TEMP_PAGE)->next;
					temp_map_page(next1);
					physaddr next2 = ((PhysMemoryBlock*)TEMP_PAGE)->next;
					size_t new_count = ((PhysMemoryBlock*)TEMP_PAGE)->size;
					temp_map_page(next2);
					((PhysMemoryBlock*)TEMP_PAGE)->prev = cur_block;
					temp_map_page(cur_block);
					((PhysMemoryBlock*)TEMP_PAGE)->next = next2;
					((PhysMemoryBlock*)TEMP_PAGE)->size += new_count;
				}
				break;
			} 
			else if (base + (count << PAGE_OFFSET_BITS) == cur_block) 
			{
				if (((PhysMemoryBlock*)TEMP_PAGE)->prev + (((PhysMemoryBlock*)TEMP_PAGE)->size << PAGE_OFFSET_BITS) == base)
				{
					physaddr next = ((PhysMemoryBlock*)TEMP_PAGE)->next;
					size_t size = ((PhysMemoryBlock*)TEMP_PAGE)->size;
					temp_map_page(((PhysMemoryBlock*)TEMP_PAGE)->prev);
					((PhysMemoryBlock*)TEMP_PAGE)->next = next;
					((PhysMemoryBlock*)TEMP_PAGE)->size = size + count;
					break;
				}
				size_t old_count = ((PhysMemoryBlock*)TEMP_PAGE)->size;
				physaddr next = ((PhysMemoryBlock*)TEMP_PAGE)->next;
				physaddr prev = ((PhysMemoryBlock*)TEMP_PAGE)->prev;
				temp_map_page(next);
				((PhysMemoryBlock*)TEMP_PAGE)->prev = base;
				temp_map_page(prev);
				((PhysMemoryBlock*)TEMP_PAGE)->next = base;
				temp_map_page(base);
				((PhysMemoryBlock*)TEMP_PAGE)->next = next;
				((PhysMemoryBlock*)TEMP_PAGE)->prev = prev;
				((PhysMemoryBlock*)TEMP_PAGE)->size = count + old_count;
				break;
			} 
			else if (cur_block > base) 
			{
				physaddr prev = ((PhysMemoryBlock*)TEMP_PAGE)->prev;
				((PhysMemoryBlock*)TEMP_PAGE)->prev = base;
				temp_map_page(prev);
				((PhysMemoryBlock*)TEMP_PAGE)->next = base;
				temp_map_page(base);
				((PhysMemoryBlock*)TEMP_PAGE)->next = cur_block;
				((PhysMemoryBlock*)TEMP_PAGE)->prev = prev;
				((PhysMemoryBlock*)TEMP_PAGE)->size = count;
				break;
			} 
			else if (((PhysMemoryBlock*)TEMP_PAGE)->next == free_phys_memory_pointer)
			{
				physaddr next = ((PhysMemoryBlock*)TEMP_PAGE)->next;
				((PhysMemoryBlock*)TEMP_PAGE)->next = base;
				temp_map_page(next);
				((PhysMemoryBlock*)TEMP_PAGE)->prev = base;
				temp_map_page(base);
				((PhysMemoryBlock*)TEMP_PAGE)->next = next;
				((PhysMemoryBlock*)TEMP_PAGE)->prev = cur_block;
				((PhysMemoryBlock*)TEMP_PAGE)->size = count;
				break;
			}
			cur_block = ((PhysMemoryBlock*)TEMP_PAGE)->next;
		} while (cur_block != free_phys_memory_pointer);
		if (base < free_phys_memory_pointer) 
		{
			free_phys_memory_pointer = base;
		}
	}
	free_page_count += count;
} 

physaddr get_physaddr(void *vaddr) 
{
	physaddr page_dir;
	asm("movl %%cr3, %0":"=a"(page_dir));

	size_t pdindex = (size_t)vaddr >> 22;
	size_t ptindex = (size_t)vaddr >> 12 & 0x3ff;

	temp_map_page(page_dir);
	physaddr page_table = ((physaddr*)TEMP_PAGE)[pdindex];

	temp_map_page(page_table);
	return ((physaddr*)TEMP_PAGE)[ptindex];
} 

physaddr alloc_phys_pages(size_t count) {
	if (free_page_count < count) return -1;
	physaddr result = -1;
	if (free_phys_memory_pointer != -1)
	{
		physaddr cur_block = free_phys_memory_pointer;
		do 
		{
			temp_map_page(cur_block);
			if (((PhysMemoryBlock*)TEMP_PAGE)->size == count) 
			{
				physaddr next = ((PhysMemoryBlock*)TEMP_PAGE)->next;
				physaddr prev = ((PhysMemoryBlock*)TEMP_PAGE)->prev;
				temp_map_page(next);
				((PhysMemoryBlock*)TEMP_PAGE)->prev = prev;
				temp_map_page(prev);
				((PhysMemoryBlock*)TEMP_PAGE)->next = next;
				if (cur_block == free_phys_memory_pointer) 
				{
					free_phys_memory_pointer = next;
					if (cur_block == free_phys_memory_pointer) 
					{
						free_phys_memory_pointer = -1;
					}
				}
				result = cur_block;
				break;
			} 
			else if (((PhysMemoryBlock*)TEMP_PAGE)->size > count) 
			{
				((PhysMemoryBlock*)TEMP_PAGE)->size -= count;
				result = cur_block + (((PhysMemoryBlock*)TEMP_PAGE)->size << PAGE_OFFSET_BITS);
				break;
			}
			cur_block = ((PhysMemoryBlock*)TEMP_PAGE)->next;	
		} while (cur_block != free_phys_memory_pointer);
		if (result != -1) 
		{
			free_page_count -= count;
		} 
	}
	return result;
}  

bool map_pages(physaddr page_dir1, void *vaddr, physaddr paddr, size_t count, unsigned int flags) 
{
	physaddr page_dir;
	asm("movl %%cr3, %0":"=a"(page_dir));

	for (; count; count--)
	{
		size_t pdindex = (size_t)vaddr >> 22;
		size_t ptindex = (size_t)vaddr >> 12 & 0x3ff;

		temp_map_page(page_dir);
		physaddr page_table = ((physaddr*)TEMP_PAGE)[pdindex];

		if (!(page_table & PAGE_PRESENT)) 
		{
			physaddr addr = alloc_phys_pages(1);
			if (addr != -1) 
			{
				temp_map_page(page_dir);
				((physaddr*)TEMP_PAGE)[pdindex] = addr | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
			} 
			else return false; 
		}
		temp_map_page(((physaddr*)TEMP_PAGE)[pdindex]);
		((physaddr*)TEMP_PAGE)[ptindex] = (paddr & ~PAGE_OFFSET_MASK) | flags;
		flush_page_cache(vaddr);

		vaddr += PAGE_SIZE;
		paddr += PAGE_SIZE;
	}

	return true;



	for (; count; count--) {
		physaddr page_table = page_dir; //0x1000
		char shift;
		for (shift = PHYADDR_BITS - PAGE_TABLE_INDEX_BITS; shift >= PAGE_OFFSET_BITS; shift -= PAGE_TABLE_INDEX_BITS) {
			unsigned int index = ((size_t)vaddr >> shift) & PAGE_TABLE_INDEX_MASK;
			temp_map_page(page_table);
			if (shift > PAGE_OFFSET_BITS) 
			{
				size_t prev_page_table = page_table;
				page_table = ((physaddr*)TEMP_PAGE)[index];
				if (!(page_table & PAGE_PRESENT)) 
				{
					physaddr addr = alloc_phys_pages(1);
					if (addr != -1) 
					{
						temp_map_page(addr);
						memset((void*)TEMP_PAGE, 0, PAGE_SIZE);
						temp_map_page(prev_page_table);
						((physaddr*)TEMP_PAGE)[index] = addr | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
						page_table = addr;
					} 
					else 
					{
						return false;
					}
				}
			} 
			else 
			{
				((physaddr*)TEMP_PAGE)[index] = (paddr & ~PAGE_OFFSET_MASK) | flags;
				flush_page_cache(vaddr);
			}
		}
		vaddr += PAGE_SIZE;
		paddr += PAGE_SIZE;
	}
	return true;
}

static inline bool is_blocks_overlapped(void *base1, size_t size1, void *base2, size_t size2) 
{
	return ((base1 >= base2) && (base1 < base2 + size2)) || ((base2 >= base1) && (base2 < base1 + size1));
}

void *alloc_virt_pages(AddressSpace *address_space, void *vaddr, physaddr paddr, size_t count, unsigned int flags) 
{
	VirtMemoryBlockType type = VMB_IO_MEMORY;
	size_t i;
    if (vaddr == NULL) 
	{
		vaddr = address_space->end - ((count) << PAGE_OFFSET_BITS);
		for (i = 0; i < address_space->block_count; i++) {
			if (is_blocks_overlapped(address_space->blocks[i].base, address_space->blocks[i].length, vaddr, count << PAGE_OFFSET_BITS)) {
				vaddr = address_space->blocks[i].base - (count << PAGE_OFFSET_BITS);
			} else {
				break;
			}
		} 
	} 
	else 
	{
		if ((vaddr >= address_space->start) && (vaddr + (count << PAGE_OFFSET_BITS) < address_space->end)) {
			for (i = 0; i < address_space->block_count; i++) {
				if (is_blocks_overlapped(address_space->blocks[i].base, address_space->blocks[i].length, vaddr, count << PAGE_OFFSET_BITS)) {
					vaddr = NULL;
					break;	
				} else if (address_space->blocks[i].base < vaddr) {
					break;
				}
			}
		} else {
			vaddr = NULL;
		}
	}
	if (vaddr != NULL) 
	{
		if (paddr == -1) 
		{
			paddr = alloc_phys_pages(count);
			type = VMB_MEMORY;
		}
		if (paddr != -1) 
		{
			if (map_pages(address_space->page_dir, vaddr, paddr, count, flags)) 
			{
				if (address_space->block_count == address_space->block_table_size) 
				{
					size_t new_size = address_space->block_table_size * sizeof(VirtMemoryBlock);
					new_size = (new_size + PAGE_OFFSET_MASK) & ~PAGE_OFFSET_MASK;
					new_size += PAGE_SIZE;
					new_size = new_size >> PAGE_OFFSET_BITS;
					if (&kernel_address_space != address_space) 
					{
						VirtMemoryBlock *new_table = alloc_virt_pages(&kernel_address_space, NULL, -1, new_size,	
							PAGE_PRESENT | PAGE_WRITABLE);
						if (new_table) 
						{
							memcpy(new_table, address_space->blocks, address_space->block_table_size * sizeof(VirtMemoryBlock));
							free_virt_pages(&kernel_address_space, address_space->blocks, 0);
							address_space->blocks = new_table;
						} 
						else 
						{
							goto fail;
						}
					} 
					else 
					{
						physaddr new_page = alloc_phys_pages(1);
						if (new_page == -1) 
						{
							goto fail;
						}
						else
						{
							VirtMemoryBlock *main_block = &(address_space->blocks[address_space->block_count - 1]);
							if (map_pages(address_space->page_dir, (void*)(main_block->base + main_block->length), new_page, 1, PAGE_PRESENT | PAGE_WRITABLE)) 
							{
								main_block->length += PAGE_SIZE;
							} 
							else 
							{
								free_phys_pages(new_page, 1);
							}
						}
					}
					address_space->block_table_size = (new_size << PAGE_OFFSET_BITS) / sizeof(VirtMemoryBlock);
				}
				memcpy(address_space->blocks + i + 1, address_space->blocks + i, \
					  (address_space->block_count - i) * sizeof(VirtMemoryBlock));
				address_space->block_count++;
				address_space->blocks[i].type = type;
				address_space->blocks[i].base = vaddr;
				address_space->blocks[i].length = count << PAGE_OFFSET_BITS;
			} 
			else 
			{
					fail:
				map_pages(address_space->page_dir, vaddr, 0, count, 0);
				free_phys_pages(paddr, count);
				vaddr = NULL;
			}
		}
	}
	return vaddr;
}

bool free_virt_pages(AddressSpace *address_space, void *vaddr, unsigned int flags) {
	size_t i;
	for (i = 0; i < address_space->block_count; i++) {
		if ((address_space->blocks[i].base <= vaddr) && (address_space->blocks[i].base + address_space->blocks[i].length > vaddr)) {
			break;
		}
	}
	if (i < address_space->block_count) {
		if (address_space->blocks[i].type = VMB_MEMORY) {
			free_phys_pages(get_physaddr(vaddr) & ~PAGE_OFFSET_MASK, address_space->blocks[i].length >> PAGE_OFFSET_BITS);
		}
		address_space->block_count--;
		memcpy(address_space->blocks + i, address_space->blocks + i + 1, (address_space->block_count - i) * sizeof(VirtMemoryBlock));
			return true;
	} else {
		return false;
	}
} 

void* kmalloc(size_t size)
{
	if (dynamic_memory.block_count == 0)
	{
		dynamic_memory.blocks[0].base = alloc_virt_pages(&kernel_address_space, NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
		dynamic_memory.blocks[0].length = size;
		dynamic_memory.block_count += 1;
		return dynamic_memory.blocks[0].base;
	}
	if((int)(((int)(dynamic_memory.blocks[0].base) & 0xfff) - size) >= 0)
	{
		memcpy(&(dynamic_memory.blocks[1]), &(dynamic_memory.blocks[0]), dynamic_memory.block_count * sizeof(DynamicMemoryBlock));
		dynamic_memory.blocks[0].base = dynamic_memory.blocks[1].base - size;
		dynamic_memory.blocks[0].length = size;
		dynamic_memory.block_count += 1;
		return dynamic_memory.blocks[0].base;		
	}
	int i = 0;
	DynamicMemoryBlock cur_block;
	DynamicMemoryBlock next_block;
	for (; i < dynamic_memory.block_count - 1; i++)
	{
		cur_block = dynamic_memory.blocks[i];
	    next_block = dynamic_memory.blocks[i + 1];
	    if (((int)(cur_block.base) & 0xfffff000) == ((int)(next_block.base) & 0xfffff000))
	    { 	
			if(cur_block.base + cur_block.length + size <= next_block.base)
			{
				memcpy(&(dynamic_memory.blocks[i + 2]), &(dynamic_memory.blocks[i + 1]), (dynamic_memory.block_count - i - 1) * sizeof(DynamicMemoryBlock));
				dynamic_memory.blocks[i + 1].base = dynamic_memory.blocks[i].base + dynamic_memory.blocks[i].length;
				dynamic_memory.blocks[i + 1].length = size;
				dynamic_memory.block_count += 1;
				return dynamic_memory.blocks[i + 1].base;
			}
		}
		else
		{	
			if (((int)(cur_block.base + cur_block.length) & 0xfffff000) + PAGE_SIZE == ((int)(next_block.base) & 0xfffff000))
			{
				if (size <= next_block.base - (cur_block.base + cur_block.length))
				{
					memcpy(&(dynamic_memory.blocks[i + 2]), &(dynamic_memory.blocks[i + 1]), (dynamic_memory.block_count - i - 1) * sizeof(DynamicMemoryBlock));
					dynamic_memory.blocks[i + 1].base = dynamic_memory.blocks[i].base + dynamic_memory.blocks[i].length;
					dynamic_memory.blocks[i + 1].length = size;
					dynamic_memory.block_count += 1;
					return dynamic_memory.blocks[i + 1].base;
				}
			}
			else if (((int)(cur_block.base + cur_block.length + size) & 0xffff) < PAGE_SIZE) { break; }
			else { continue; }			
		}
	}
	if (i == dynamic_memory.block_count - 1)
	{
		if (size <= PAGE_SIZE - ((int)(dynamic_memory.blocks[i].base) & 0xfff) - dynamic_memory.blocks[i].length)
		{
			dynamic_memory.blocks[i + 1].base = dynamic_memory.blocks[i].base + dynamic_memory.blocks[i].length;
			dynamic_memory.blocks[i + 1].length = size;
			dynamic_memory.block_count += 1;
			return dynamic_memory.blocks[i + 1].base;
		} 
		else
		{
			void* base = alloc_virt_pages(&kernel_address_space, NULL, -1, size / PAGE_SIZE + ((size % PAGE_SIZE > 0)? 1: 0), PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
			if((((int)base) & 0xfffff000) < (((int)dynamic_memory.blocks[0].base) & 0xFFFFF000))
			{
				memcpy(&(dynamic_memory.blocks[1]), &(dynamic_memory.blocks[0]), dynamic_memory.block_count * sizeof(DynamicMemoryBlock));
				dynamic_memory.blocks[0].base = base;
				dynamic_memory.blocks[0].length = size;
				dynamic_memory.block_count += 1;
				return dynamic_memory.blocks[0].base;	
			}
			for (i = 0; i < dynamic_memory.block_count - 1; i++)
			{
				cur_block = dynamic_memory.blocks[i];
	    		next_block = dynamic_memory.blocks[i + 1];
	    		if ((base < next_block.base) && (base > cur_block.base))
	    		{
	    			memcpy(&(dynamic_memory.blocks[i + 2]), &(dynamic_memory.blocks[i + 1]), (dynamic_memory.block_count - i - 1) * sizeof(DynamicMemoryBlock));
					dynamic_memory.blocks[i + 1].base = dynamic_memory.blocks[i].base + dynamic_memory.blocks[i].length;
					dynamic_memory.blocks[i + 1].length = size;
					dynamic_memory.block_count += 1;
					return dynamic_memory.blocks[i + 1].base;
	    		}
			}	
			dynamic_memory.blocks[i + 2].base = base;
			dynamic_memory.blocks[i + 2].length = size;
			dynamic_memory.block_count += 1;
			return dynamic_memory.blocks[i + 2].base;
		}
	}
	return NULL;
}

void kfree(void* ptr)
{
	DynamicMemoryBlock cur_block;
	DynamicMemoryBlock next_block;
	for (int i = 0; i < dynamic_memory.block_count; i++)
	{
		cur_block = dynamic_memory.blocks[i];
		if (cur_block.base == ptr)
		{
			if (i == 0)
			{
				if (((int)(cur_block.base) & 0xfffff000) != ((int)(dynamic_memory.blocks[i + 1].base) & 0xfffff000))
				{
					free_virt_pages(&kernel_address_space, (void*)((int)(cur_block.base) & 0xfffff000), 0);
				}
	
			}
			else if (i == dynamic_memory.block_count - 1)
			{
				if (((int)(cur_block.base) & 0xfffff000) != ((int)(dynamic_memory.blocks[i - 1].base + dynamic_memory.blocks[i - 1].length) & 0xfffff000))
				{
					free_virt_pages(&kernel_address_space, (void*)((int)(cur_block.base) & 0xfffff000), 0);
				}
			}
			else
			{
				if ((((int)(cur_block.base) & 0xfffff000) != ((int)(dynamic_memory.blocks[i + 1].base) & 0xfffff000)) \
					&& (((int)(cur_block.base) & 0xfffff000) != ((int)(dynamic_memory.blocks[i - 1].base + dynamic_memory.blocks[i - 1].length) & 0xfffff000)))
				{
					free_virt_pages(&kernel_address_space, (void*)((int)(cur_block.base) & 0xfffff000), 0);
				}
			}
			memcpy(&(dynamic_memory.blocks[i]), &(dynamic_memory.blocks[i + 1]), (dynamic_memory.block_count - i) * sizeof(DynamicMemoryBlock));	
			dynamic_memory.block_count -= 1;
			break;
		}
	}
}