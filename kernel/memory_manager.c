#include "include/stdlib.h"
#include "include/memory_manager.h"
#include "include/tty.h"

size_t free_page_count;
physaddr free_phys_memory_pointer;
physaddr kernel_page_dir;
size_t memory_size;
VirtMemory virt_memory;
DynamicMemory dynamic_memory;


void init_memory_manager(void* memory_map) 
{
	//init physical memory
	asm("mov %%cr3, %0":"=a"(kernel_page_dir));
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
	virt_memory.block_count = 0;
	virt_memory.blocks_table = (VirtMemoryBlock*)0x0;
	virt_memory.table_size = 1;

	map_pages(virt_memory.blocks_table, alloc_phys_pages(virt_memory.table_size), 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
	virt_memory.blocks_table[virt_memory.block_count].base = 0;
	virt_memory.blocks_table[virt_memory.block_count].size = 1;
	virt_memory.block_count++;
	
	map_pages((void*)KERNEL_CODE_BASE, get_physaddr((void*)KERNEL_CODE_BASE),
		((size_t)KERNEL_DATA_BASE - (size_t)KERNEL_CODE_BASE) >> PAGE_OFFSET_BITS, PAGE_PRESENT | PAGE_GLOBAL);
	map_pages((void*)KERNEL_DATA_BASE, \
			  get_physaddr((void*)KERNEL_DATA_BASE), \
			  ((size_t)KERNEL_END - (size_t)KERNEL_DATA_BASE) >> PAGE_OFFSET_BITS, \
			  PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
	map_pages((void*)KERNEL_PAGE_TABLE, get_physaddr((void*)KERNEL_PAGE_TABLE), 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);	

	dynamic_memory.block_count = 0;
	dynamic_memory.blocks = alloc_virt_pages(NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
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

physaddr get_physaddr(void *vaddr) 
{
	physaddr page_dir;
	asm("mov %%cr3, %0":"=a"(page_dir));

	size_t pdindex = (size_t)vaddr >> 22;
	size_t ptindex = (size_t)vaddr >> 12 & 0x3ff;

	temp_map_page(page_dir);
	physaddr page_table = ((physaddr*)TEMP_PAGE)[pdindex];

	temp_map_page(page_table);
	return ((physaddr*)TEMP_PAGE)[ptindex];
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
	physaddr page_dir;
	asm("mov %%cr3, %0":"=a"(page_dir));

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
}
bool unmap_pages(void *vaddr, size_t count)
{
	physaddr page_dir;
	asm("mov %%cr3, %0":"=a"(page_dir));

	for (; count; count--)
	{
		size_t pdindex = (size_t)vaddr >> 22;
		size_t ptindex = (size_t)vaddr >> 12 & 0x3ff;

		temp_map_page(page_dir);
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
	int i = 0;
	void *ret = (void*)-1;
	VirtMemoryBlock *cur_block = &(virt_memory.blocks_table[i]);
	VirtMemoryBlock *next_block = &(virt_memory.blocks_table[i + 1]);
	
	if ((size_t)(cur_block->base) >= (size << PAGE_OFFSET_BITS))
	{
		memcpy(cur_block + 1, cur_block, virt_memory.block_count * sizeof(VirtMemoryBlock));

		cur_block->base = 0;
		cur_block->size = size;

		virt_memory.block_count += 1;

		ret = cur_block->base;
	}
	for (; i < virt_memory.block_count - 1; i++, cur_block = &(virt_memory.blocks_table[i]), next_block = &(virt_memory.blocks_table[i + 1]))
	{
		if (cur_block->base + ((cur_block->size + size) << PAGE_OFFSET_BITS) <= next_block->base)
		{
			memcpy(next_block + 1, next_block, (virt_memory.block_count - i) * sizeof(VirtMemoryBlock));

			next_block->base = cur_block->base + (cur_block->size << PAGE_OFFSET_BITS);
			next_block->size = size;

			virt_memory.block_count += 1;

			ret = next_block->base;			
		}
	}
	if (ret == (void*)-1)
	{
		next_block->base = cur_block->base + (cur_block->size << PAGE_OFFSET_BITS);
		next_block->size = size;
		virt_memory.block_count += 1;

		ret = next_block->base;
	}	
	return ret;
}
size_t del_virt_block(void* base)
{
	VirtMemoryBlock *temp;
	for (int i = 0; i < virt_memory.block_count; i++)
	{
		temp = &(virt_memory.blocks_table[i]);
		if (temp->base == base)
		{
			size_t size = temp->size;
			memcpy(temp, temp + 1, (virt_memory.block_count - i) * sizeof(VirtMemoryBlock));
			virt_memory.block_count -= 1;

			return size;
		}
	}
	return 0;
}

void* alloc_virt_pages(void *vaddr, physaddr paddr, size_t count, unsigned int flags) 
{
	if (vaddr == NULL)
	{
		if ((virt_memory.block_count + 1) * sizeof(VirtMemoryBlock) > virt_memory.table_size << PAGE_OFFSET_BITS)
		{
			void *old_table = virt_memory.blocks_table;
			del_virt_block(virt_memory.blocks_table);

			virt_memory.table_size += 1;
			void *new_table = add_virt_block(virt_memory.table_size);

			for (int i = 0; i < virt_memory.table_size; i++)
			{
				map_pages(new_table + i * PAGE_SIZE, alloc_phys_pages(1), 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
			}
			memcpy(new_table, old_table, virt_memory.table_size - 1);

			for (int i = 0; i < virt_memory.table_size - 1; i++)
			{
				free_phys_pages(get_physaddr(old_table + i * PAGE_SIZE), 1);
			}
			unmap_pages(old_table, virt_memory.table_size - 1);
		}
		vaddr = add_virt_block(count);
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
	return NULL;
}
size_t free_virt_pages(void *vaddr) 
{
	size_t count = del_virt_block(vaddr);
	for (int i = 0; i < count; i++)
	{
		free_phys_pages(get_physaddr(vaddr + i * PAGE_SIZE), 1);
	}
	unmap_pages(vaddr, count);

	if (virt_memory.table_size << PAGE_OFFSET_BITS - virt_memory.block_count * sizeof(VirtMemoryBlock) >= PAGE_SIZE)
	{
		free_phys_pages(get_physaddr(virt_memory.blocks_table + (virt_memory.table_size - 1) * PAGE_SIZE), 1);
		unmap_pages(virt_memory.blocks_table + (virt_memory.table_size - 1) * PAGE_SIZE, 1);

		del_virt_block(virt_memory.blocks_table);

		virt_memory.table_size -= 1;
		add_virt_block(virt_memory.table_size);
	}
	return count;
} 
void* kmalloc(size_t size)
{
	if (dynamic_memory.block_count == 0)
	{
		dynamic_memory.blocks[0].base = alloc_virt_pages(NULL, -1, 1, PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
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
			void* base = alloc_virt_pages(NULL, -1, size / PAGE_SIZE + ((size % PAGE_SIZE > 0)? 1: 0), PAGE_PRESENT | PAGE_WRITABLE | PAGE_GLOBAL);
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
					free_virt_pages((void*)((int)(cur_block.base) & 0xfffff000));
				}
	
			}
			else if (i == dynamic_memory.block_count - 1)
			{
				if (((int)(cur_block.base) & 0xfffff000) != ((int)(dynamic_memory.blocks[i - 1].base + dynamic_memory.blocks[i - 1].length) & 0xfffff000))
				{
					free_virt_pages((void*)((int)(cur_block.base) & 0xfffff000));
				}
			}
			else
			{
				if ((((int)(cur_block.base) & 0xfffff000) != ((int)(dynamic_memory.blocks[i + 1].base) & 0xfffff000)) \
					&& (((int)(cur_block.base) & 0xfffff000) != ((int)(dynamic_memory.blocks[i - 1].base + dynamic_memory.blocks[i - 1].length) & 0xfffff000)))
				{
					free_virt_pages((void*)((int)(cur_block.base) & 0xfffff000));
				}
			}
			memcpy(&(dynamic_memory.blocks[i]), &(dynamic_memory.blocks[i + 1]), (dynamic_memory.block_count - i) * sizeof(DynamicMemoryBlock));	
			dynamic_memory.block_count -= 1;
			break;
		}
	}
}
