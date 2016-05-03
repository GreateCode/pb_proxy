#include "memory_pool.h"
#include <stdio.h>

Memory_Pool *Memory_Pool::pool_ = NULL;

Memory_Pool::Memory_Pool():
	pool_list_()
{
	for(int i = 0; i < MAX_INDEX; i++){
		Mem_Ctrl *p = (Mem_Ctrl *)malloc(sizeof(Mem_Ctrl));
		p->mem_list = NULL;
		p->mem_free = NULL;
		p->total_size = 0;
		p->free_size = 0;
		p->index = i;
		pool_list_[i] = p;
	}
}

Memory_Pool::~Memory_Pool(){

}

Memory_Pool *Memory_Pool::get_instance(){
	if(pool_ == NULL)
		pool_ = new Memory_Pool();
	return pool_;
}

char *Memory_Pool::get_memory(int data_size){
	LOCK
	char *data = NULL;
	int size = MEM_ALIGN(data_size + OFFSET, BOUNDARY);
	int index = (size >> BOUNDARY_INDEX) - 1;
	if(pool_list_[index]->free_size != 0)
		data = get_free_memory(index);
	else
		data = get_new_memory(index);
	UNLOCK
	return data;
}

void Memory_Pool::free_memory(char *data){
	LOCK
	Mem_Node *node = (Mem_Node *)(data - OFFSET);
	int index = node->index;
	if(pool_list_[index]->free_size >= MAX_FREE_SIZE){
		free(node);
		pool_list_[index]->total_size--;
		UNLOCK
		return;
	}
	node->next = pool_list_[index]->mem_free;
	pool_list_[index]->mem_free = node;
	pool_list_[index]->free_size++;
	UNLOCK
}

char *Memory_Pool::get_free_memory(int index){
	Mem_Node *node = pool_list_[index]->mem_free;
	pool_list_[index]->mem_free = node->next;
	node->next = NULL;
	pool_list_[index]->free_size--;
	int size = (index + 1) << BOUNDARY_INDEX;
	memset(node->begin, 0, size - OFFSET);
	return node->begin;
}

char *Memory_Pool::get_new_memory(int index){
	int size = (index + 1) << BOUNDARY_INDEX;
	Mem_Node *node = (Mem_Node *)malloc(size);
	node->index = index;
	node->next = NULL;
	node->begin = (char *)node + OFFSET;
	memset(node->begin, 0, size - OFFSET);
	pool_list_[index]->total_size++;
	return node->begin;
}

