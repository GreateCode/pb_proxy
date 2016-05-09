#ifndef __MEMORY_POOL__
#define __MEMORY_POOL__

#include <stdlib.h>
#include <string.h>
#include "pthread_sync.h"

#define MAX_INDEX 50
#define BOUNDARY 1024
#define BOUNDARY_INDEX 10
#define MAX_FREE_SIZE 1024

#define MEM_ALIGN(size, boundary) \
	(((size) + (boundary) - 1) &~((boundary) - 1))

typedef struct _Mem_Node {
	_Mem_Node *next;
	int index;
	char *begin;
	char *data;
}__attribute__((packed)) Mem_Node;

#define OFFSET (sizeof(Mem_Node*) + sizeof(int) + sizeof(char *))

typedef struct _Mem_Ctrl {
	_Mem_Node *mem_list;
	_Mem_Node *mem_free;
	size_t total_size;
	size_t free_size;
	int index;
}Mem_Ctrl;

class Memory_Pool {
public:
	static Memory_Pool *get_instance();
	char *get_memory(int data_size);
	void free_memory(char *data);
private:
	Memory_Pool();
	~Memory_Pool();
	char *get_free_memory(int index);
	char *get_new_memory(int index);
private:
	static Memory_Pool *pool_;
	_Mem_Ctrl *pool_list_[MAX_INDEX];
	PTHREAD_SYNC;
};

#define MEMORY_POOL Memory_Pool::get_instance()
#define MEM_MALLOC(size) Memory_Pool::get_instance()->get_memory(size)
#define MEM_FREE(data) Memory_Pool::get_instance()->free_memory(data)

class Memory_Tool {
public:
	Memory_Tool(char **data, int size){
		*data = MEM_MALLOC(size);
		data_ = *data;
		size_ = size;
	}
	~Memory_Tool(){
		MEM_FREE(data_);
	}
private:
	char *data_;
	int size_;
};

#define MEM_INIT(data, size) Memory_Tool tool(&(data), (size))

#endif

