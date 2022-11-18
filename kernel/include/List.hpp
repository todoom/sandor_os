#ifndef LIST_H
#define LIST_H

#include "stdlib.h"
#include "memory_manager.h"

//TODO
#define foreach(i, list) for(item_t<type> *i = list->head; i != NULL; i = i->next)

template <typename type>
class item_t
{
public:
	type value;
	item_t *next;
};

template <typename type>
class List
{
public:
	type* list;
	int length;
	item_t<type> *head;
	item_t<type> *tail;

	List()
	{
		this->list = (type*)(kmalloc(sizeof(type)));
		this->length = 0;
		this->head = NULL;
		this->tail = NULL;
	}

	type get_value(size_t index)
	{
		int i = 0;
		foreach(item, this)
		{
			if (i == index) return (type)(item->value);
			i++;
		}
		return (type)NULL;
	}

	void append(item_t<type>* item)
	{
		if (this->length == 0)
		{
			this->head = item;
			this->tail = item;
		}
		else
		{
			(this->tail)->next = item;
			this->tail = item;
		}
		this->length++;
	}

	item_t<type>* insert(type value)
	{
		item_t<type>* item = (item_t<type>*)(kmalloc(sizeof(item_t<type>)));
		memcpy(&(item->value), &value, sizeof(type));	
		item->next = NULL;
		this->append(item);
		return item;
	}

	item_t<type>* pop()
	{
		return (item_t<type>*)0;
	}

	bool remove(int index)
	{
		item_t<type> *prev_item, *cur_item = this->head;
		int i = 0;
		while ((i < index) && (cur_item->next != NULL))
		{
			i++;
			prev_item = cur_item;
			cur_item = cur_item->next;
		}
		if (i == 0)
		{
			this->head = cur_item->next;
			kfree(cur_item);
			this->length -= 1;
			if (length == 1)
			{
				this->tail = this->head;
			}
			return true;
		}
		else if (i == this->length)
		{
			kfree(cur_item);
			prev_item->next = NULL;
			this->tail = prev_item;
			this->length -= 1;
			return true;
		}
		else if (i == index)
		{
			prev_item->next = cur_item->next;
			kfree(cur_item);
			this->length -= 1;
			return true;
		}
		return false;
	}
};


#endif