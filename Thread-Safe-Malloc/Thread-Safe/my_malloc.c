#include "my_malloc.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

//define a lock
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
__thread block_fl * head_free_nolock = NULL;
__thread block_fl * tail_free_nolock = NULL;

//define the head and tail of the free list
block_fl * head_free_lock = NULL;
block_fl * tail_free_lock = NULL;


//Best Fit malloc
//Search the free list to find the smallest block that is large enough
//If no block is large enough, call sbrk to get a new block
void * bf_malloc(size_t size, block_fl* head_free, block_fl* tail_free, int lock_flag){
    if (head_free == NULL) {
        //if the free list is empty, call sbrk to get a new block
        //LOCK
        block_fl * new_block;
        if(lock_flag == 1){
            new_block = sbrk(size + sizeof(block_fl));
        }
        else{
            pthread_mutex_lock(&lock);
            new_block = sbrk(size + sizeof(block_fl));
            pthread_mutex_unlock(&lock);
        }
        //UNLOCK
        new_block->size = size;
        new_block->next = NULL;
        new_block->prev = NULL;
        return (void *)new_block + sizeof(block_fl);
    }
    else if (head_free != NULL) {
        //if the free list is not empty, find the smallest block that is large enough
        block_fl * current = head_free;
        block_fl * best_block = NULL;
        //traverse the free list to find the smallest block that is large enough
        while (current != NULL) {
            if (current->size >= size) {
                if (current->size == size) {
                    best_block = current;
                    break;//if the block is exactly the size we need, break the loop
                }
                if (best_block == NULL) {
                    best_block = current;
                }
                else if (current->size < best_block->size){
                    best_block = current;
                }
            }
            current = current->next;
        }
        if (best_block != NULL) {
            if (best_block->size - size >= sizeof(block_fl) + 1) {
                split(best_block, size, head_free, tail_free);
                best_block->next = NULL;
                best_block->prev = NULL;
                return (void *)best_block + sizeof(block_fl);
            }
            else{
                remove_block(best_block, head_free, tail_free);
                best_block->next = NULL;
                best_block->prev = NULL;
                return (void *)best_block + sizeof(block_fl);
            }
        }
        else{
            //LOCK
            block_fl * new_block;
            if(lock_flag == 1){
                new_block = sbrk(size + sizeof(block_fl));
            }
            else{
                pthread_mutex_lock(&lock);
                new_block = sbrk(size + sizeof(block_fl));
                pthread_mutex_unlock(&lock);
            }
            //UNLOCK
            new_block->size = size;
            new_block->next = NULL;
            new_block->prev = NULL;
            return (void *)new_block + sizeof(block_fl);
        }
    }
}

//Best Fit free
//Merge the newly freed region with any currently free adjacent regions
void bf_free(void * ptr, block_fl* head_free, block_fl* tail_free) {
    //get the block that is going to be freed
    block_fl * block = (void *)ptr - sizeof(block_fl);
    //block->free_flag = 1;
    //insert the block into the free list
    insert_block(block, head_free, tail_free);
    //coalesce the free blocks
    coalesce(block, head_free, tail_free);
}

//Coalesce free blocks
//e.g. Use immediate coalescing to merge all the free blocks when a new block is freed
void coalesce(block_fl * block, block_fl* head_free, block_fl* tail_free) {
    //if the block is not the tail of the free list, check the next block
    if (block->next != NULL) {
        //if the next block is free, merge the two blocks according to the address
        if ((void *)block + sizeof(block_fl) + block->size == (void *)block->next) {
            block->size = block->size + block->next->size + sizeof(block_fl);
            block->next = block->next->next;
            if (block->next != NULL) {
                block->next->prev = block;
            }
            //if the next block is the tail of the free list, update the tail
            if (block->next == NULL) {
                tail_free = block;
            }
        }
        
    }
    //if the block is not the head of the free list, check the previous block
    if (block->prev != NULL) {
        //if the previous block is free, merge the two blocks
        if ((void *)block->prev + sizeof(block_fl) + block->prev->size == (void *)block) {
            block->prev->size = block->prev->size + block->size + sizeof(block_fl);
            block->prev->next = block->next;
            if (block->next != NULL) {
                block->next->prev = block->prev;
            }
            //if the previous block is the tail of the free list, update the tail
            if (block->next == NULL) {
                tail_free = block->prev;
            }
        }
    }
}

//Split a block into two blocks when the size of the block is larger than the requested size
//Use split function when the remaining size is enought to hold metadata and a pointer
void split(block_fl * block, size_t size, block_fl* head_free, block_fl* tail_free) {
    //create a new block
    block_fl * new_block = (void *)block + sizeof(block_fl) + size;
    new_block->size = block->size - size - sizeof(block_fl);
    new_block->next = block->next;
    new_block->prev = block->prev;
    //remove the original block from the free list
    remove_block(block, head_free, tail_free);
    //insert the new block into the free list
    insert_block(new_block, head_free, tail_free);
    //update the size of the original block
    block->size = size;
}

//Remove a block from the free list
void remove_block(block_fl * block, block_fl* head_free, block_fl* tail_free) {
    //if there is only one block in the free list, set the head and tail to NULL
    if (head_free == block && tail_free == block) {
        head_free = NULL;
        tail_free = NULL;
    }
    //update the head and tail of the free list
    else if (block == head_free) {
        head_free = block->next;
        head_free->prev = NULL;
    }
    else if (block == tail_free) {
        tail_free = block->prev;
        tail_free->next = NULL;
    }
    //update the next and prev pointers of the adjacent blocks
    else {
        block->prev->next = block->next;
        block->next->prev = block->prev;
    }
}

//Insert a block into the free list according to the address
void insert_block(block_fl * block, block_fl* head_free, block_fl* tail_free) {
    //if the free list is empty, set the head and tail to the block
    if (head_free == NULL && tail_free == NULL) {
        head_free = block;
        tail_free = block;
    }
    //if the node is ahead of the head, insert it to the head
    if ((void *)block < (void *)head_free) {
        block->next = head_free;
        block->prev = NULL;
        head_free->prev = block;
        head_free = block;
    }
    //if the node is between the head and the tail, insert it between the head and the tail
    else if ((void *)block > (void *)head_free && (void *)block < (void *)tail_free) {
        block_fl * current = head_free;
        while (current != NULL) {
            if ((void *)block > (void *)current && (void *)block < (void *)current->next) {
                block->next = current->next;
                block->prev = current;
                current->next->prev = block;
                current->next = block;
                break;
            }
            current = current->next;
        }
    }
    //if the node is after the tail, insert it to the tail
    else if ((void *)block > (void *)tail_free) {
        block->next = NULL;
        block->prev = tail_free;
        tail_free->next = block;
        tail_free = block;
    }
    
}

//Create thread-safe malloc for the best-fit allocator by using mutex lock
void *ts_malloc_lock(size_t size) {
    int lock_flag = 1;
    pthread_mutex_lock(&lock);
    void * ptr = bf_malloc(size, head_free_lock, tail_free_lock, lock_flag);
    pthread_mutex_unlock(&lock);
    return ptr;
}


//Create thread-safe free for the besåt-fit allocator by using mutex lock
void ts_free_lock(void *ptr) {
    pthread_mutex_lock(&lock);
    bf_free(ptr, head_free_lock, tail_free_lock);
    pthread_mutex_unlock(&lock);
}

//Thread Safe malloc/free: non-locking version
void *ts_malloc_nolock(size_t size){
    int lock_flag = 0;
    void* ptr = bf_malloc(size, head_free_nolock, tail_free_nolock, lock_flag);
    return ptr;
}

void ts_free_nolock(void *ptr){
    bf_free(ptr, head_free_nolock, tail_free_nolock);
}