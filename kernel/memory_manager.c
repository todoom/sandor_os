#include "include/stdlib.h"
#include "include/memory_manager.h"
#include "include/tty.h"
#include "include/multiboot.h"

size_t free_page_count;
physaddr free_phys_memory_pointer;
physaddr kernel_page_dir;
size_t memory_size;
VirtMemory kernel_virt_blocks;
DynamicMemory dynamic_memory;
void *TEMP_PAGE;
size_t *TEMP_PAGE_INFO;
AddressSpace *cur_address_space;
AddressSpace kernel_address_space;
AddressSpace user_address_space;

void init_memory_manager(multiboot_uint32_t memory_map) 
{
	//init physical memory
	asm("mov %%cr3, %0":"=a"(kernel_page_dir));
	memory_size = 0x100000;
	free_page_count = 0;
	free_phys_memory_pointer = -1;
	multiboot_memory_map_t *entry;

	TEMP_PAGE = (void*)((size_t)KERNEL_BASE_VMA & (PAGE_TABLE_INDEX_MASK << (PAGE_OFFSET_BITS + PAGE_TABLE_INDEX_BITS))) + (PAGE_TABLE_INDEX_MASK << PAGE_OFFSET_BITS);
	TEMP_PAGE_INFO = (size_t*)(KERNEL_PAGE_TABLE + ((((size_t)TEMP_PAGE >> PAGE_OFFSET_BITS) & PAGE_TABLE_INDEX_MASK) << 0)); //TODO

	for (entry = (multiboot_memory_map_t*)memory_map; entry->type; entry++) 
	{
		if ((entry->type == 1) && (entry->addr >= 0x100000)) 
		{	
			if (entry->addr == (size_t)KERNEL_BASE_LMA)
			{
				entry->addr += (size_t)KERNEL_END - (size_t)KERNEL_BASE_VMA;
			}
			free_phys_pages(entry->addr, entry->len >> PAGE_OFFSET_BITS);
			memory_size += entry->len;
		}
	}
	//kernel address space initialize
	cur_address_space = &kernel_address_space;
	kernel_address_space.page_dir = kernel_page_dir;
	kernel_address_space.start = KERNEL_ADDRESS_SPACE_START;
	kernel_address_space.end = KERNEL_ADDRESS_SPACE_END;

	//virtual memory initialize
	kernel_address_space.virt_memory.block_count = 0;
	kernel_address_space.virt_memory.blocks = (VirtMemoryBlock*)KERNEL_VIRT_BLOCK_TABLE;
	kernel_address_space.virt_memory.table_size = 1;

	map_pages(kernel_address_space.virt_memory.blocks, alloc_phys_pages(kernel_address_space.virt_memory.table_size), 1, 0xfff);
	//map_pages(KERNEL_BASE_VMA, KERNEL_BASE_LMA, ((size_t)KERNEL_END - (size_t)KERNEL_BASE_VMA) >> PAGE_OFFSET_BITS, PAGE_PRESENT | PAGE_GLOBAL);	
	map_pages(TEMP_PAGE, (physaddr)0x0, 1, 0xfff);	

	kernel_address_space.virt_memory.blocks[kernel_address_space.virt_memory.block_count].base = ((size_t)KERNEL_BASE_VMA);
	kernel_address_space.virt_memory.blocks[kernel_address_space.virt_memory.block_count].size = ((size_t)KERNEL_END - (size_t)KERNEL_BASE_VMA) >> PAGE_OFFSET_BITS;
	kernel_address_space.virt_memory.block_count++;

	kernel_address_space.virt_memory.blocks[kernel_address_space.virt_memory.block_count].base = kernel_address_space.virt_memory.blocks;
	kernel_address_space.virt_memory.blocks[kernel_address_space.virt_memory.block_count].size = kernel_address_space.virt_memory.table_size;
	kernel_address_space.virt_memory.block_count++;
	
	kernel_address_space.virt_memory.blocks[kernel_address_space.virt_memory.block_count].base = TEMP_PAGE;
	kernel_address_space.virt_memory.blocks[kernel_address_space.virt_memory.block_count].size = 1;
	kernel_address_space.virt_memory.block_count++;

	//dynamic memory initialize
	kernel_address_space.dynamic_memory.block_count = 0;
	kernel_address_space.dynamic_memory.table_size = 1;
	kernel_address_space.dynamic_memory.blocks = alloc_virt_pages(NULL, -1, kernel_address_space.dynamic_memory.table_size, 0xfff);
	kernel_address_space.dynamic_memory.heap_size = 10;
	kernel_address_space.dynamic_memory.heap = alloc_virt_pages(NULL, -1, kernel_address_space.dynamic_memory.heap_size, 0xfff);
	
	//user address space initialize
	user_address_space.page_dir = clone_directory(kernel_address_space.page_dir);
	user_address_space.start = USER_ADDRESS_SPACE_START;
	user_address_space.end = USER_ADDRESS_SPACE_END;
	user_address_space.virt_memory = kernel_address_space.virt_memory;
	user_address_space.dynamic_memory = kernel_address_space.dynamic_memory;
}

static inline void flush_page_cache(void *addr) 
{
	asm("invlpg (,%0,)"::"a"(addr));
}

void temp_map_page(physaddr page) 
{
	*((physaddr*)TEMP_PAGE_INFO) = (page & ~PAGE_OFFSET_MASK) | 0xfff;
	flush_page_cache((void*)TEMP_PAGE);
}

physaddr get_physaddr(void *vaddr) 
{
	physaddr cur_page_dir = cur_address_space->page_dir;

	size_t pdindex = (size_t)vaddr >> 22;
	size_t ptindex = (size_t)vaddr >> 12 & 0x3ff;

	temp_map_page(cur_page_dir);
	physaddr page_table = ((physaddr*)TEMP_PAGE)[pdindex];

	temp_map_page(page_table);
	return (((physaddr*)TEMP_PAGE)[ptindex] & ~0xFFF) + ((size_t)vaddr & 0xFFF);
} 

physaddr alloc_phys_pages(size_t count) 
{
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
					free_phys_memory_pointer = prev;
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
			cur_block = ((PhysMemoryBlock*)TEMP_PAGE)->prev;	
		} while (cur_block != free_phys_memory_pointer);
		if (result != -1) 
		{
			free_page_count -= count;
		} 
	}
	return result;
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
			if (base > cur_block + (((PhysMemoryBlock*)TEMP_PAGE)->size << PAGE_OFFSET_BITS))
			{
				physaddr next = ((PhysMemoryBlock*)TEMP_PAGE)->next; 
				((PhysMemoryBlock*)TEMP_PAGE)->next = base;
				temp_map_page(next);
				((PhysMemoryBlock*)TEMP_PAGE)->prev = base;
				temp_map_page(base);
				((PhysMemoryBlock*)TEMP_PAGE)->next = next;
				((PhysMemoryBlock*)TEMP_PAGE)->prev = cur_block;
				((PhysMemoryBlock*)TEMP_PAGE)->size = count;
				if (cur_block == free_phys_memory_pointer) 
				{
					free_phys_memory_pointer = base;
				}	
				break;
			}
			else if (cur_block + (((PhysMemoryBlock*)TEMP_PAGE)->size << PAGE_OFFSET_BITS) == base) 
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
				physaddr prev = ((PhysMemoryBlock*)TEMP_PAGE)->prev;
				temp_map_page(prev);
				if (prev + (((PhysMemoryBlock*)TEMP_PAGE)->size << PAGE_OFFSET_BITS) == base)
				{
					temp_map_page(cur_block);
					physaddr next = ((PhysMemoryBlock*)TEMP_PAGE)->next;
					size_t size = ((PhysMemoryBlock*)TEMP_PAGE)->size;
					temp_map_page(prev);	
					((PhysMemoryBlock*)TEMP_PAGE)->next = next;
					((PhysMemoryBlock*)TEMP_PAGE)->size += size + count;
					temp_map_page(next);
					((PhysMemoryBlock*)TEMP_PAGE)->prev = prev;
					if (cur_block == free_phys_memory_pointer) 
					{
						free_phys_memory_pointer = prev;
					}
					break;
				}
				temp_map_page(cur_block);
				size_t old_count = ((PhysMemoryBlock*)TEMP_PAGE)->size;
				physaddr next = ((PhysMemoryBlock*)TEMP_PAGE)->next;
				temp_map_page(next);
				((PhysMemoryBlock*)TEMP_PAGE)->prev = base;
				temp_map_page(prev);
				((PhysMemoryBlock*)TEMP_PAGE)->next = base;
				temp_map_page(base);
				((PhysMemoryBlock*)TEMP_PAGE)->next = next;
				((PhysMemoryBlock*)TEMP_PAGE)->prev = prev;
				((PhysMemoryBlock*)TEMP_PAGE)->size = count + old_count;
				if (cur_block == free_phys_memory_pointer) free_phys_memory_pointer = base;
				break;
			} 
			cur_block = ((PhysMemoryBlock*)TEMP_PAGE)->prev;
		} while (cur_block != free_phys_memory_pointer);
	}
	mark:
	free_page_count += count;
} 

bool map_pages(void *vaddr, physaddr paddr, size_t count, unsigned int flags) 
{
	physaddr cur_page_dir = cur_address_space->page_dir;

	for (; count; count--)
	{
		size_t pdindex = (size_t)vaddr >> 22;
		size_t ptindex = (size_t)vaddr >> 12 & 0x3ff;

		temp_map_page(cur_page_dir);
		physaddr page_table = ((physaddr*)TEMP_PAGE)[pdindex];

		
		if (!(page_table & PAGE_PRESENT)) 
		{
			physaddr addr = alloc_phys_pages(1);
			if (addr != -1) 
			{
				temp_map_page(cur_page_dir);
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
}
bool unmap_pages(void *vaddr, size_t count)
{
	physaddr cur_page_dir = cur_address_space->page_dir;

	for (; count; count--)
	{
		size_t pdindex = (size_t)vaddr >> 22;
		size_t ptindex = (size_t)vaddr >> 12 & 0x3ff;

		temp_map_page(cur_page_dir);
		physaddr page_table = ((physaddr*)TEMP_PAGE)[pdindex];

		if (!(page_table & PAGE_PRESENT)) return false; 
		
		temp_map_page(((physaddr*)TEMP_PAGE)[pdindex]);
		((physaddr*)TEMP_PAGE)[ptindex] = 0;
		flush_page_cache(vaddr);

		vaddr += PAGE_SIZE;
	}
	return true;
}

void* add_virt_block(size_t size)
{
	VirtMemory *cur_virt_memory = &(cur_address_space->virt_memory);
	int i = 0;
	VirtMemoryBlock *cur_block = &(cur_virt_memory->blocks[i]);
	VirtMemoryBlock *nxt_block = &(cur_virt_memory->blocks[i + 1]);
	if ((size_t)(cur_block->base) >= (size_t)cur_address_space->start + (size << PAGE_OFFSET_BITS))
	{
		memcpy(cur_block + 1, cur_block, cur_virt_memory->block_count * sizeof(VirtMemoryBlock));

		cur_block->base = (size_t)cur_address_space->start;
		cur_block->size = size;

		cur_virt_memory->block_count += 1;
		return cur_block->base;
	}
	for (; i < cur_virt_memory->block_count - 1; i++, cur_block = &(cur_virt_memory->blocks[i]), nxt_block = &(cur_virt_memory->blocks[i + 1]))
	{
		if (cur_block->base + ((cur_block->size + size) << PAGE_OFFSET_BITS) > cur_address_space->end)
		{
			return (void*)NULL;
		}
		if (cur_block->base + ((cur_block->size + size) << PAGE_OFFSET_BITS) <= nxt_block->base)
		{
			memcpy(nxt_block + 1, nxt_block, (cur_virt_memory->block_count - i) * sizeof(VirtMemoryBlock));

			nxt_block->base = cur_block->base + (cur_block->size << PAGE_OFFSET_BITS);
			nxt_block->size = size;

			cur_virt_memory->block_count += 1;

			return nxt_block->base;			
		}
	}
	if (cur_block->base + ((cur_block->size + size) << PAGE_OFFSET_BITS) > cur_address_space->end)
	{
		return (void*)NULL;
	}
	nxt_block->base = cur_block->base + (cur_block->size << PAGE_OFFSET_BITS);
	nxt_block->size = size;
	cur_virt_memory->block_count += 1;

	return nxt_block->base;
}
size_t del_virt_block(void* base)
{
	VirtMemory *cur_virt_memory = &(cur_address_space->virt_memory);
	VirtMemoryBlock *cur_block;
	for (int i = 0; i < cur_virt_memory->block_count; i++)
	{
		cur_block = &(cur_virt_memory->blocks[i]);
		if ((size_t)(cur_block->base) == (size_t)base)
		{
			size_t size = cur_block->size;
			memcpy(cur_block, cur_block + 1, (cur_virt_memory->block_count - i) * sizeof(VirtMemoryBlock));
			cur_virt_memory->block_count -= 1;

			return size;
		}
	}
	return 0;
}

void* alloc_virt_pages(void *vaddr, physaddr paddr, size_t count, unsigned int flags) 
{
	VirtMemory *cur_virt_memory = &(cur_address_space->virt_memory);
	if ((cur_virt_memory->block_count + 1) * sizeof(VirtMemoryBlock) > cur_virt_memory->table_size << PAGE_OFFSET_BITS)
	{
		//TODO
		void *old_table = cur_virt_memory->blocks;
		del_virt_block(cur_virt_memory->blocks);

		cur_virt_memory->table_size += 1;
		void *new_table = add_virt_block(cur_virt_memory->table_size);
		for (int i = 0; i < cur_virt_memory->table_size; i++)
		{
			map_pages(new_table + i * PAGE_SIZE, alloc_phys_pages(1), 1, 0xfff);
		}
		memcpy(new_table, old_table, (cur_virt_memory->table_size - 1) << PAGE_OFFSET_BITS);
		for (int i = 0; i < cur_virt_memory->table_size - 1; i++)
		{
			free_phys_pages(get_physaddr(old_table + i * PAGE_SIZE), 1);
		}
		unmap_pages(old_table, cur_virt_memory->table_size - 1);
		cur_virt_memory->blocks = new_table;
	}
	if (vaddr == NULL)
	{
		vaddr = add_virt_block(count);	
	}
	if (paddr == -1)
	{
		for (int i = 0; i < count; i++)
		{
			map_pages(vaddr + i * PAGE_SIZE, alloc_phys_pages(1), 1, flags);
		}
	}
	else
	{
		for (int i = 0; i < count; i++)
		{
			map_pages(vaddr + i * PAGE_SIZE, paddr + i * PAGE_SIZE, 1, flags);
		}
	}
	return vaddr;
}
size_t free_virt_pages(void *vaddr) 
{
	VirtMemory *cur_virt_memory = &(cur_address_space->virt_memory);
	size_t count = del_virt_block(vaddr);
	for (int i = 0; i < count; i++)
	{
		free_phys_pages(get_physaddr(vaddr + i * PAGE_SIZE), 1);
	}
	unmap_pages(vaddr, count);

	if (((cur_virt_memory->table_size << PAGE_OFFSET_BITS) - cur_virt_memory->block_count * sizeof(VirtMemoryBlock)) >= PAGE_SIZE)
	{
		free_phys_pages(get_physaddr(cur_virt_memory->blocks + (cur_virt_memory->table_size - 1) * PAGE_SIZE), 1);
		unmap_pages(cur_virt_memory->blocks + (cur_virt_memory->table_size - 1) * PAGE_SIZE, 1);

		del_virt_block(cur_virt_memory->blocks);

		cur_virt_memory->table_size -= 1;
		add_virt_block(cur_virt_memory->table_size);
	}
	return count;
} 

void* kmalloc(size_t size)
{
	DynamicMemory *cur_dynamic_memory = &(cur_address_space->dynamic_memory);

	if ((cur_dynamic_memory->block_count + 1) * sizeof(DynamicMemoryBlock) > cur_dynamic_memory->table_size << PAGE_OFFSET_BITS)
	{
		size_t old_table_size = cur_dynamic_memory->table_size;
		void *old_table = cur_dynamic_memory->blocks;
		void *new_table = alloc_virt_pages(NULL, -1, old_table_size + 1, 0xfff);

		if (new_table == NULL) return NULL;
		memcpy(new_table, old_table, old_table_size << PAGE_OFFSET_BITS);

		cur_dynamic_memory->blocks = new_table;
		cur_dynamic_memory->table_size = old_table_size + 1;

		free_virt_pages(old_table);
	}
	if (cur_dynamic_memory->block_count == 0)
	{
		cur_dynamic_memory->blocks[0].base = cur_dynamic_memory->heap;
		cur_dynamic_memory->blocks[0].size = size;

		cur_dynamic_memory->block_count += 1;

		return cur_dynamic_memory->blocks[0].base;
	}
	size_t i = 0;
	DynamicMemoryBlock *cur_block = &(cur_dynamic_memory->blocks[i]);
	DynamicMemoryBlock *nxt_block = &(cur_dynamic_memory->blocks[i + 1]);
	if (cur_dynamic_memory->blocks[0].base >= (size_t)(cur_dynamic_memory->heap) + size)
	{	
		memcpy(cur_block + 1, cur_block, cur_dynamic_memory->block_count * sizeof(VirtMemoryBlock));

		cur_block->base = cur_dynamic_memory->blocks[0].base - size;
		cur_block->size = size;

		cur_dynamic_memory->block_count += 1;
		return cur_block->base;
	}
	for (; i < cur_dynamic_memory->block_count - 1; i++, cur_block = &(cur_dynamic_memory->blocks[i]), nxt_block = &(cur_dynamic_memory->blocks[i + 1]))
	{
		if ((cur_block->base + cur_block->size + size) <= nxt_block->base)
		{
			memcpy(nxt_block + 1, nxt_block, (cur_dynamic_memory->block_count - i) * sizeof(VirtMemoryBlock));

			nxt_block->base = cur_block->base + cur_block->size;
			nxt_block->size = size;

			cur_dynamic_memory->block_count += 1;

			return nxt_block->base;			
		}
	}
	if ((size_t)(cur_block->base) + (((size_t)(cur_block->size) + size)) > ((size_t)(cur_dynamic_memory->heap) + ((size_t)(cur_dynamic_memory->heap_size) << PAGE_OFFSET_BITS)))
	{
		return NULL;
	}
	nxt_block->base = cur_block->base + cur_block->size;
	nxt_block->size = size;
	cur_dynamic_memory->block_count += 1;

	return nxt_block->base;
}
bool kfree(void* ptr)
{
	DynamicMemory *cur_dynamic_memory = &(cur_address_space->dynamic_memory);

	size_t i = 0;
	DynamicMemoryBlock *cur_block = &(cur_dynamic_memory->blocks[i]);

	for (; i < cur_dynamic_memory->block_count; i++)
	{
		if ((size_t)(cur_block->base) == (size_t)ptr)
		{
			memcpy(cur_block, cur_block + 1, (cur_dynamic_memory->block_count - i) * sizeof(VirtMemoryBlock));
			cur_dynamic_memory->block_count -= 1;

			return true;
		}
	}
	return false;
}

void flush_cr3(physaddr pd)
{
	asm("movl %%eax, %%cr3"::"a"(pd));
}

void change_address_space(AddressSpace *address_space)
{
	cur_address_space = address_space;
	flush_cr3(cur_address_space->page_dir);
}

page_directory_t clone_directory(page_directory_t src)
{
	page_directory_t clone_dir = alloc_phys_pages(1);
	page_directory_t cloneable_dir = alloc_virt_pages(NULL, src, 1, 0xfff);
	
	temp_map_page(clone_dir);	
	memset(TEMP_PAGE, 0, 0x1000);
	for (size_t i = 0; i < 1024; i++)
	{
		if (kernel_address_space.page_dir[i] == cloneable_dir[i])
		{
			((page_directory_t*)TEMP_PAGE)[i] = cloneable_dir[i];
		}
		else
		{
			//TODO
		}
	}

	del_virt_block(cloneable_dir);
	unmap_pages(cloneable_dir, 1);

	return clone_dir;
}

AddressSpace* create_address_space()
{
	// AddressSpace *new_address_space = kmalloc(sizeof(AddressSpace));
	// new_address_space.page_dir = clone_directory(kernel_address_space.page_dir);
	// new_address_space.start = USER_ADDRESS_SPACE_START;
	// new_address_space.end = USER_ADDRESS_SPACE_END;
	// new_address_space.virt_memory = kernel_address_space.virt_memory;
	// new_address_space.dynamic_memory = kernel_address_space.dynamic_memory;
}
/*
void* kmalloc(size_t size)
{
	if (cur_dynamic_memory->block_count == 0)
	{
		cur_dynamic_memory->blocks[0].base = alloc_virt_pages(NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
		cur_dynamic_memory->blocks[0].length = size;
		cur_dynamic_memory->block_count += 1;
		return cur_dynamic_memory->blocks[0].base;
	}
	if((int)(((int)(cur_dynamic_memory->blocks[0].base) & PAGE_OFFSET_MASK) - size) >= 0)
	{
		memcpy(&(cur_dynamic_memory->blocks[1]), &(cur_dynamic_memory->blocks[0]), cur_dynamic_memory->block_count * sizeof(DynamicMemoryBlock));
		cur_dynamic_memory->blocks[0].base = cur_dynamic_memory->blocks[1].base - size;
		cur_dynamic_memory->blocks[0].length = size;
		cur_dynamic_memory->block_count += 1;
		return cur_dynamic_memory->blocks[0].base;		
	}
	int i = 0;
	DynamicMemoryBlock cur_block;
	DynamicMemoryBlock next_block;
	for (; i < cur_dynamic_memory->block_count - 1; i++)
	{
		cur_block = cur_dynamic_memory->blocks[i];
	    next_block = cur_dynamic_memory->blocks[i + 1];
	    if (((int)(cur_block.base) & ~PAGE_OFFSET_MASK) == ((int)(next_block.base) & ~PAGE_OFFSET_MASK))
	    { 	
			if(cur_block.base + cur_block.length + size <= next_block.base)
			{
				memcpy(&(cur_dynamic_memory->blocks[i + 2]), &(cur_dynamic_memory->blocks[i + 1]), (cur_dynamic_memory->block_count - i - 1) * sizeof(DynamicMemoryBlock));
				cur_dynamic_memory->blocks[i + 1].base = cur_dynamic_memory->blocks[i].base + cur_dynamic_memory->blocks[i].length;
				cur_dynamic_memory->blocks[i + 1].length = size;
				cur_dynamic_memory->block_count += 1;
				return cur_dynamic_memory->blocks[i + 1].base;
			}
		}
		else
		{	
			if (((int)(cur_block.base + cur_block.length) & ~PAGE_OFFSET_MASK) + PAGE_SIZE == ((int)(next_block.base) & ~PAGE_OFFSET_MASK))
			{
				if (size <= next_block.base - (cur_block.base + cur_block.length))
				{
					memcpy(&(cur_dynamic_memory->blocks[i + 2]), &(cur_dynamic_memory->blocks[i + 1]), (cur_dynamic_memory->block_count - i - 1) * sizeof(DynamicMemoryBlock));
					cur_dynamic_memory->blocks[i + 1].base = cur_dynamic_memory->blocks[i].base + cur_dynamic_memory->blocks[i].length;
					cur_dynamic_memory->blocks[i + 1].length = size;
					cur_dynamic_memory->block_count += 1;
					return cur_dynamic_memory->blocks[i + 1].base;
				}
			}
			else if (((int)(cur_block.base + cur_block.length + size) & 0xffff) < PAGE_SIZE) { break; }
			else { continue; }			
		}
	}
	if (i == cur_dynamic_memory->block_count - 1)
	{
		if (size <= PAGE_SIZE - ((int)(cur_dynamic_memory->blocks[i].base) & PAGE_OFFSET_MASK) - cur_dynamic_memory->blocks[i].length)
		{
			cur_dynamic_memory->blocks[i + 1].base = cur_dynamic_memory->blocks[i].base + cur_dynamic_memory->blocks[i].length;
			cur_dynamic_memory->blocks[i + 1].length = size;
			cur_dynamic_memory->block_count += 1;
			return cur_dynamic_memory->blocks[i + 1].base;
		} 
		else
		{
			void* base = alloc_virt_pages(NULL, -1, size / PAGE_SIZE + ((size % PAGE_SIZE > 0)? 1: 0), PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
			if((((int)base) & ~PAGE_OFFSET_MASK) < (((int)cur_dynamic_memory->blocks[0].base) & ~PAGE_OFFSET_MASK))
			{
				memcpy(&(cur_dynamic_memory->blocks[1]), &(cur_dynamic_memory->blocks[0]), cur_dynamic_memory->block_count * sizeof(DynamicMemoryBlock));
				cur_dynamic_memory->blocks[0].base = base;
				cur_dynamic_memory->blocks[0].length = size;
				cur_dynamic_memory->block_count += 1;
				return cur_dynamic_memory->blocks[0].base;	
			}
			for (i = 0; i < cur_dynamic_memory->block_count - 1; i++)
			{
				cur_block = cur_dynamic_memory->blocks[i];
	    		next_block = cur_dynamic_memory->blocks[i + 1];
	    		if ((base < next_block.base) && (base > cur_block.base))
	    		{
	    			memcpy(&(cur_dynamic_memory->blocks[i + 2]), &(cur_dynamic_memory->blocks[i + 1]), (cur_dynamic_memory->block_count - i - 1) * sizeof(DynamicMemoryBlock));
					cur_dynamic_memory->blocks[i + 1].base = cur_dynamic_memory->blocks[i].base + cur_dynamic_memory->blocks[i].length;
					cur_dynamic_memory->blocks[i + 1].length = size;
					cur_dynamic_memory->block_count += 1;
					return cur_dynamic_memory->blocks[i + 1].base;
	    		}
			}	
			cur_dynamic_memory->blocks[i + 2].base = base;
			cur_dynamic_memory->blocks[i + 2].length = size;
			cur_dynamic_memory->block_count += 1;
			return cur_dynamic_memory->blocks[i + 2].base;
		}
	}
	return NULL;}
void kfree(void* ptr)
{
	DynamicMemoryBlock cur_block;
	DynamicMemoryBlock next_block;
	for (int i = 0; i < cur_dynamic_memory->block_count; i++)
	{
		cur_block = cur_dynamic_memory->blocks[i];
		if (cur_block.base == ptr)
		{
			if (i == 0)
			{
				if (((int)(cur_block.base) & ~PAGE_OFFSET_MASK) != ((int)(cur_dynamic_memory->blocks[i + 1].base) & ~PAGE_OFFSET_MASK))
				{
					free_virt_pages((void*)((int)(cur_block.base) & ~PAGE_OFFSET_MASK));
				}
	
			}
			else if (i == cur_dynamic_memory->block_count - 1)
			{
				if (((int)(cur_block.base) & ~PAGE_OFFSET_MASK) != ((int)(cur_dynamic_memory->blocks[i - 1].base + cur_dynamic_memory->blocks[i - 1].length) & ~PAGE_OFFSET_MASK))
				{
					free_virt_pages((void*)((int)(cur_block.base) & ~PAGE_OFFSET_MASK));
				}
			}
			else
			{
				if ((((int)(cur_block.base) & ~PAGE_OFFSET_MASK) != ((int)(cur_dynamic_memory->blocks[i + 1].base) & ~PAGE_OFFSET_MASK)) \
					&& (((int)(cur_block.base) & ~PAGE_OFFSET_MASK) != ((int)(cur_dynamic_memory->blocks[i - 1].base + cur_dynamic_memory->blocks[i - 1].length) & ~PAGE_OFFSET_MASK)))
				{
					free_virt_pages((void*)((int)(cur_block.base) & ~PAGE_OFFSET_MASK));
				}
			}
			memcpy(&(cur_dynamic_memory->blocks[i]), &(cur_dynamic_memory->blocks[i + 1]), (cur_dynamic_memory->block_count - i) * sizeof(DynamicMemoryBlock));	
			cur_dynamic_memory->block_count -= 1;
			break;
		}
	}}
*/