#include "my_malloc.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

//define the head and tail of the free list
block_fl * head_free = NULL;
block_fl * tail_free = NULL;

//First Fit malloc
void * ff_malloc(size_t size){
    if (head_free == NULL) {
        //if the free list is empty, call sbrk to get a new block
        block_fl * new_block = sbrk(size + sizeof(block_fl));
        new_block->size = size;
        new_block->next = NULL;
        new_block->prev = NULL;
        new_block->free_flag = 0;
        return (void *)new_block + sizeof(block_fl);//address of the top of the block
    }
    else if (head_free != NULL) {
        //if the free list is not empty, find the first block that is large enough
        block_fl * current = head_free;
        while (current != NULL) {
            if (current->size >= size) {
                //if the block is large enough, split the block if the remaining size is large enough
                if (current->size - size >= sizeof(block_fl) + 1) {
                    split(current, size);//include remove_block and insert_block
                    current-> free_flag = 0;
                    current->next = NULL;
                    current->prev = NULL;
                    return (void *)current + sizeof(block_fl);
                }
                //if the remaining size is not large enough, remove the block from the free list
                else{
                    remove_block(current);
                    current->free_flag = 0;
                    current->next = NULL;
                    current->prev = NULL;
                    return (void *)current + sizeof(block_fl);
                }
            }
            current = current->next;
        }
        //if no block is large enough, call sbrk to get a new block
        block_fl * new_block = sbrk(size + sizeof(block_fl));
        new_block->size = size;
        new_block->next = NULL;
        new_block->prev = NULL;
        new_block->free_flag = 0;
        return (void *)new_block + sizeof(block_fl);
    }
}

//Best Fit malloc
//Search the free list to find the smallest block that is large enough
//If no block is large enough, call sbrk to get a new block
void * bf_malloc(size_t size){
    if (head_free == NULL) {
        //if the free list is empty, call sbrk to get a new block
        block_fl * new_block = sbrk(size + sizeof(block_fl));
        new_block->size = size;
        new_block->next = NULL;
        new_block->prev = NULL;
        new_block->free_flag = 0;
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
                split(best_block, size);
                best_block-> free_flag = 0;
                best_block->next = NULL;
                best_block->prev = NULL;
                return (void *)best_block + sizeof(block_fl);
            }
            else{
                remove_block(best_block);
                best_block->free_flag = 0;
                best_block->next = NULL;
                best_block->prev = NULL;
                return (void *)best_block + sizeof(block_fl);
            }
        }
        else{
            block_fl * new_block = sbrk(size + sizeof(block_fl));
            new_block->size = size;
            new_block->next = NULL;
            new_block->prev = NULL;
            new_block->free_flag = 0;
            return (void *)new_block + sizeof(block_fl);
        }
    }
}

//First Fit free
//Merge the newly freed region with any currently free adjacent regions
void ff_free(void * ptr) {
    //get the block that is going to be freed
    block_fl * block = (void *)ptr - sizeof(block_fl);
    block->free_flag = 1;
    //insert the block into the free list
    insert_block(block);
    //coalesce the free blocks
    coalesce(block);
}

//Best Fit free
//Same as First Fit free
void bf_free(void * ptr) {
    return ff_free(ptr);
}

//return the size of the largest free block in bytes
unsigned long get_largest_free_data_segment_size() {
    if (head_free == NULL) {
        return 0;
    }
    else{
        block_fl * current = head_free;
        unsigned long largest_size = 0;
        while (current != NULL) {
            if (current->size > largest_size) {
                largest_size = current->size;
            }
            current = current->next;
        }
        return largest_size;
    }
}

//return the total size of free blocks in bytes
unsigned long get_total_free_size() {
    if (head_free == NULL) {
        return 0;
    }
    else{
        block_fl * current = head_free;
        unsigned long total_size = 0;
        while (current != NULL) {
            total_size = total_size + current->size;
            current = current->next;
        }
        return total_size;
    }
}

//Coalesce free blocks
//e.g. Use immediate coalescing to merge all the free blocks when a new block is freed
void coalesce(block_fl * block) {
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
void split(block_fl * block, size_t size) {
    //create a new block
    block_fl * new_block = (void *)block + sizeof(block_fl) + size;
    new_block->size = block->size - size - sizeof(block_fl);
    new_block->next = block->next;
    new_block->prev = block->prev;
    new_block->free_flag = 1;
    //remove the original block from the free list
    remove_block(block);
    //insert the new block into the free list
    insert_block(new_block);
    //update the size of the original block
    block->size = size;
}

//Remove a block from the free list
void remove_block(block_fl * block) {
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
void insert_block(block_fl * block) {
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