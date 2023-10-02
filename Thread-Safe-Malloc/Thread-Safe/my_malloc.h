#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

//Create an explicit free list (double linked list) to keep track of free blocks
//Each block contains pointers, size, and a flag to indicate if it is free or not
typedef struct block{
    struct block * next;
    struct block * prev;
    size_t size;
} block_fl;//free list block


//Best Fit malloc
void * bf_malloc(size_t size, block_fl* head_free, block_fl* tail_free, int lock_flag);

//Best Fit free
void bf_free(void * ptr, block_fl* head_free, block_fl* tail_free);

//Coalesce free blocks
//e.g. Use immediate coalescing to merge all the free blocks when a new block is freed
void coalesce(block_fl * block, block_fl* head_free, block_fl* tail_free);

//Split a block into two blocks when the size of the block is larger than the requested size
//Use split function when the remaining size is enought to hold metadata and a pointer
void split(block_fl * block, size_t size, block_fl* head_free, block_fl* tail_free);

//Remove a block from the free list
void remove_block(block_fl * block, block_fl* head_free, block_fl* tail_free);

//Insert a block into the free list
void insert_block(block_fl * block, block_fl* head_free, block_fl* tail_free);

//Thread Safe malloc/free: locking version
void *ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);

//Thread Safe malloc/free: non-locking version
void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);
