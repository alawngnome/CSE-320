/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"


/*HELPER METHOD -initialize the heap with prologue, wilderness, epilogue, free_lists
*/
void malloc_initialize_heap() {
    char *heap_pointer = (char *) sf_mem_grow(); //create the first page of the memory
    heap_pointer += 48; //skip 6 rows in the very beginning of the heap(padding-1 for prev_footer)
    struct sf_block *new_prolouge = (sf_block *) heap_pointer;
    new_prolouge->header = 64;//(64>>2); //adds block size to its space
    new_prolouge->header|=3; //sets the two LSBs to be 1 and 1


    heap_pointer += 64; //skipping 8 rows (size of prologue)
    struct sf_block *new_wilderness = (sf_block *) heap_pointer;
    new_wilderness->header = 3968;
    new_wilderness->header |= 2;
    new_wilderness->prev_footer = new_prolouge->header;


    heap_pointer = (char *)sf_mem_end() - 16; //making space for the epilogue
    struct sf_block *new_epilouge = (sf_block *) heap_pointer;
    new_epilouge->header = 1; //size of 8 + alloc bit of 1. Initialize prv_alloc to be 0
    new_epilouge->prev_footer = new_wilderness->header;

    for(int i = 0; i < NUM_FREE_LISTS-1; i++) { //initialize free lists in the array
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
    }
    //set the last array element to point towards wilderness block
    sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next = new_wilderness;
    sf_free_list_heads[NUM_FREE_LISTS-1].body.links.prev = new_wilderness;
    new_wilderness->body.links.next = &sf_free_list_heads[NUM_FREE_LISTS-1];
    new_wilderness->body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS-1];
}

/*HELPER METHOD - finds the smallest possible free list index to begin traversal at.
*/
int malloc_find_start(size_t size){
    size /= 64; //dividing to get the multiple of 64;
    if(size == 1)
        return 0;
    else if(size == 2)
        return 1;
    else if(size == 3)
        return 2;

    int one = 3;
    int two = 5;
    int i = 3;
    while(two <= 34) {
        if(one < size && size <= two)
            return i;

        int one_temp = one;
        one = two;
        two = one_temp + two;
        i++;
    }
    return 8;
}

/*HELPER METHOD FOR malloc_split_block - finds and inserts back a free block into the segregated free lists
*/
void malloc_split_insert(struct sf_block *block) {

    int index = malloc_find_start(block->header|~3); //find the array index based off the blocksize
    struct sf_block *block_pointer = &sf_free_list_heads[index]; //gets the head of of the list
    if(block_pointer->body.links.next == block_pointer &&
    block_pointer->body.links.prev == block_pointer) { //if the list is empty
        block_pointer->body.links.next = block;
        block_pointer->body.links.prev = block;
        block->body.links.next = block_pointer;
        block->body.links.prev = block_pointer;
        return;
    }
    //insert to the head of the free list
    struct sf_block *prev_temp = block_pointer->body.links.next->body.links.prev;
    block_pointer->body.links.next->body.links.prev = block;
    block->body.links.next = block_pointer->body.links.next;
    block->body.links.prev = prev_temp;
    prev_temp->body.links.next = block;

}

/* HELPER METHOD - splits a new free block of size length off an original free block in the heap
*/
struct sf_block *malloc_split_block(int size, struct sf_block *original_block, int is_wilderness) {
    if(original_block->header - size < 64) //if splitting the size will create splinters
        size = original_block->header|~3; //use the entire original_block

    //remove original block from free_list - we will reallocate it once blocksize changes
    struct sf_block *prev_temp = original_block->body.links.prev;
    original_block->body.links.prev->body.links.next = original_block->body.links.next;
    original_block->body.links.next->body.links.prev = prev_temp;
    //original block now has size in header and setting it to be allocated
    original_block->header = (size)|1; //alloc bit set to 1
    //creating the new block size based off the remainder;
    char *heap_pointer = (char *)original_block + size - 8; //creates a memory address for the new upper block to be placed
    struct sf_block *new_block = (struct sf_block *) heap_pointer; //creates the new upper block
    new_block->header = (original_block->header - size)|2; //setting header with prv_alloc bit of 1 - original_block will be allocated
    //setting the prev_footer of the new_block
    heap_pointer = (char *)original_block + (original_block->header-size) - 8; //going past the size of the new_block
    struct sf_block *prev_footer_block = (struct sf_block *) heap_pointer;
    prev_footer_block->prev_footer = new_block->header;
    //insert the new block into a free list
    malloc_split_insert(new_block);
    //check if splitting from a wilderness block, elsewise put original_block into a free list
    if(is_wilderness){
        sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next = original_block;
        sf_free_list_heads[NUM_FREE_LISTS-1].body.links.prev = original_block;
        original_block->body.links.next = &sf_free_list_heads[NUM_FREE_LISTS-1];
        original_block->body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS-1];
    }
    else
        malloc_split_insert(original_block); //re-insert with new block_size

    return new_block;
}

/* HELPER FUNCTION - first-fit search for a free block, starting at the malloc_find_start position free list
 * If not found, goes onto the next position, and then the wilderness block
 * If wilderness too small or doesn't exist, use sf_mem_grow
 * If cannot allocate block, set sf_errno to ENOMEM and malloc to return null
*/
struct sf_block *malloc_search_insert(int size){

    int start_position = malloc_find_start(size); //index of the free list to traverse through

    struct sf_block *block_pointer = &sf_free_list_heads[start_position];

    if(block_pointer->body.links.next == block_pointer->body.links.prev) { //if list is empty
        start_position++;
        block_pointer = &sf_free_list_heads[start_position]; //move to next free list in array
    } else { //if we can traverse through the list
        block_pointer = block_pointer->body.links.next; //start by moving off the sentinel node
        while(block_pointer != &sf_free_list_heads[start_position]){ //loop until we hit sentinel node again
            if((block_pointer->header|~3) >= size) //if we find a suitably large block
                return malloc_split_block(size, block_pointer, 0);
        }
    }
}


void *sf_malloc(size_t size) {
    if (size == 0)
        return NULL;
    size += 8; //adding header size in bytes
    if(size%64) //if not already a multiple of 64
        size = (size|63) + 1; //round up to next multiple of 64
    if(size<64) //minimum block size is 64
        return NULL;

    if(sf_mem_start() == sf_mem_end()) { //if first malloc call
        malloc_initialize_heap();
    }
    //sf_show_heap();
    return malloc_search_insert(size);
    return NULL;
}

void sf_free(void *pp) {
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    return NULL;
}

void *sf_memalign(size_t size, size_t align) {
    return NULL;
}
