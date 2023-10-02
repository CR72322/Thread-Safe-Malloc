#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

//Create an explicit free list (double linked list) to keep track of free blocks
//Each block contains pointers, size, and a flag to indicate if it is free or not
typedef struct block{
    struct block * next;
    struct block * prev;
    size_t size;
    int free_flag;
} block_fl;//free list block

//First Fit malloc
void * ff_malloc(size_t size);

//Best Fit malloc
void * bf_malloc(size_t size);

//First Fit free
void ff_free(void * ptr);

//Best Fit free
void bf_free(void * ptr);

//return the size of the largest free block
unsigned long get_largest_free_data_segment_size();//in bytes

//return the total size of free blocks
unsigned long get_total_free_size();//in bytes

//Coalesce free blocks
//e.g. Use immediate coalescing to merge all the free blocks when a new block is freed
void coalesce(block_fl * block);

//Split a block into two blocks when the size of the block is larger than the requested size
//Use split function when the remaining size is enought to hold metadata and a pointer
void split(block_fl * block, size_t size);

//Remove a block from the free list
void remove_block(block_fl * block);

//Insert a block into the free list
void insert_block(block_fl * block);