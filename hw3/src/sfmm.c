/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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

    int index = malloc_find_start(block->header&~3); //find the array index based off the blocksize
    struct sf_block *block_pointer = &sf_free_list_heads[index]; //gets the head of of the list

    //insert to the head of the free list
    struct sf_block *prev_temp = block_pointer->body.links.next->body.links.prev;
    block_pointer->body.links.next->body.links.prev = block;
    block->body.links.next = block_pointer->body.links.next;
    block->body.links.prev = prev_temp;
    prev_temp->body.links.next = block;

}
/*HELPER METHOD - Removes the current block from the linked-list
*/
void malloc_remove_block(struct sf_block *original_block){
    original_block->body.links.prev->body.links.next = original_block->body.links.next;
    original_block->body.links.next->body.links.prev = original_block->body.links.prev;
}

/* HELPER METHOD - splits a new free block of size length off an original free block in the heap
*/
struct sf_block *malloc_split_block(size_t size, struct sf_block *original_block, int is_wilderness) {
    int entire_block_used = 0;
    if(original_block->header - size < 64){ //if splitting the size will create splinters
        size = original_block->header&~3; //use the entire original_block
        entire_block_used = 1;
    }
    //remove original block from free_list - we will reallocate it once blocksize changes
    malloc_remove_block(original_block);
    //original block now has size in header and setting it to be allocated
    size_t temp_original_header = original_block->header;
    size_t temp_original_blocksize = original_block->header&~3;
    original_block->header = (size)|1; //alloc bit set to 1
    original_block->header |= temp_original_header&2; //moving the prv_alloc bit from old header
    if(!entire_block_used) {
        //creating the new block size based off the remainder;
        char *heap_pointer = ((char *)original_block) + size; //creates a memory address for the new upper block to be placed
        struct sf_block *new_block = (struct sf_block *) heap_pointer; //creates the new upper block
        new_block->header = (temp_original_blocksize-size)|2; //prv_alloc = 1, original_block will be allocated
        new_block->prev_footer = original_block->header;
        //setting the prev_footer of the new_block
        heap_pointer += (temp_original_blocksize-size); //going past the size of the new_block
        struct sf_block *prev_footer_block = (struct sf_block *) heap_pointer;
        prev_footer_block->prev_footer = new_block->header;
        //check if splitting from a wilderness block, elsewise put original_block into a free list
        if(is_wilderness){
            sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next = new_block;
            sf_free_list_heads[NUM_FREE_LISTS-1].body.links.prev = new_block;
            new_block->body.links.next = &sf_free_list_heads[NUM_FREE_LISTS-1];
            new_block->body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS-1];
        }
        else
            malloc_split_insert(new_block); //re-insert with new block_size
    }
    return original_block;
}

/* HELPER FUNCTION - first-fit search for a free block, starting at the malloc_find_start position free list
 * If not found, goes onto the next position, and then the wilderness block
 * If wilderness too small or doesn't exist, use sf_mem_grow
 * If cannot allocate block, set sf_errno to ENOMEM and malloc to return null
*/
struct sf_block *malloc_search_insert(int size){

    int start_position = malloc_find_start(size); //index of the free list to traverse through

    struct sf_block *block_pointer = &sf_free_list_heads[start_position];

    //loop through the free lists until we reach the wilderness block
    while(block_pointer != &sf_free_list_heads[NUM_FREE_LISTS-1]) {
        if(block_pointer->body.links.next != block_pointer->body.links.prev) { //if we can traverse through the list
            block_pointer = block_pointer->body.links.next; //start by moving off the sentinel node
            while(block_pointer != &sf_free_list_heads[start_position]){ //loop until we hit sentinel node again
                if((block_pointer->header&~3) >= size) //if we find a suitably large block
                    return malloc_split_block(size, block_pointer, 0);
                block_pointer = block_pointer->body.links.next;
            }
        }
        start_position++;
        block_pointer = &sf_free_list_heads[start_position];
    }
    //if we reach the wilderness block without a suitably large block found yet
    if(sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next == &sf_free_list_heads[NUM_FREE_LISTS-1] &&
    sf_free_list_heads[NUM_FREE_LISTS-1].body.links.prev == &sf_free_list_heads[NUM_FREE_LISTS-1]){
        //the wilderness block is not in the last index of the array
    }
    else if(size > (sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next->header&~3)){
        //the wilderness block is too small
    } else { //if space can be allocated from the wilderness
        return malloc_split_block(size, sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next, 1);
    }
    //last option is to call sf_mem_grow
    //saving the epilogue header to move to the new end
    char *temp_epilogue_address = (char *)sf_mem_end() - 16;
    struct sf_block *temp_epilogue = (struct sf_block *) temp_epilogue_address;
    size_t epilogue_header = temp_epilogue->header;
    while(1) { //try to coalesce block and satisfy request
        //check if sf_mem_grow fails
        if(sf_mem_grow() == NULL)
            break; //break out of while loop
        //increase size of wilderness block by new page of memory generated
        sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next->header += PAGE_SZ;
        //try to grow another page of memory to add to the wilderness
        if(sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next->header < size)
            continue;
        //elsewise use the enbiggened wilderness block to satisfy the request
        return malloc_split_block(size, sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next, 1);
        //inserting the epilouge at the end
        temp_epilogue_address = (char *)sf_mem_end() - 16;
        struct sf_block *new_epilouge = (sf_block *) temp_epilogue_address;
        new_epilouge->header = 1; //size of 8 + alloc bit of 1. Initialize prv_alloc to be 0
        new_epilouge->prev_footer = epilogue_header;
        //or is it sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next->header
    }
    //if sf_mem_grow fails
    sf_errno = ENOMEM;
    return NULL;
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
    //return payload of the allocated type
    struct sf_block *allocated_block = malloc_search_insert(size);
    if(allocated_block == NULL) //if allocated_block is multiple of 64
        return NULL;
    return (void *)(allocated_block->body.payload);
}

/*HELPER FUNCTION - Attempts to coalesce a freed block and insert it back into the free list
*/
void free_coalesce(struct sf_block *current_block) {
    //getting the next block
    char *block_pointer = (char *)current_block + (current_block->header&~3);
    struct sf_block *next_block = (struct sf_block *)block_pointer;

    //if both next and prev blocks are allocated
    if(current_block->header&2 && next_block->header&1){
        next_block->prev_footer = current_block->header; //setting prev_footer
        next_block->header = next_block->header&~2; //setting prv_alloc block to be 0
        malloc_split_insert(current_block);
        return;
    }
    //if prev is allocated but next block is free
    else if(current_block->header&2 && !(next_block->header&1)) {
        //set header of current block and footer of next block is now combined size
        size_t new_header = ((current_block->header&~3) + (next_block->header&~3))|2; //prv_alloc = 1, alloc = 0
        current_block->header = new_header;
        block_pointer = (char *)current_block + (new_header&~3);
        struct sf_block *new_next_block = (struct sf_block *) block_pointer;
        new_next_block->prev_footer = new_header;
        malloc_remove_block(next_block);
        //if wilderness block
        if(next_block->body.links.next == &sf_free_list_heads[NUM_FREE_LISTS-1]){
            sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next = current_block;
            sf_free_list_heads[NUM_FREE_LISTS-1].body.links.prev = current_block;
            current_block->body.links.next = &sf_free_list_heads[NUM_FREE_LISTS-1];
            current_block->body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS-1];
        } else {
            malloc_split_insert(current_block);
        }
        return;
    }
    //if prev is free but next block is allocated
    else if(!(current_block->header&2) && next_block->header&1) {
        char *block_pointer = (char *)current_block - (current_block->prev_footer&~3);
        struct sf_block *prev_block = (struct sf_block *)block_pointer;

        size_t new_header = ((current_block->header&~3) + (prev_block->header&~3))
        |(prev_block->header&2); //prv_alloc = prev_block's prv_alloc bit, alloc = 0
        prev_block->header = new_header;
        block_pointer = (char *)prev_block + (new_header&~3);
        struct sf_block *new_next_block = (struct sf_block *) block_pointer;
        new_next_block->prev_footer = new_header;
        malloc_remove_block(prev_block);
        malloc_split_insert(prev_block);
        return;
    }
    //if both next and prev are free
    else if(!(current_block->header&2) && !(next_block->header&1)) {
        char *block_pointer = (char *)current_block - (current_block->prev_footer&~3);
        struct sf_block *prev_block = (struct sf_block *)block_pointer;

        size_t new_header = ((prev_block->header&~3) + (current_block->header&~3) + (next_block->header&~3))
        |(prev_block->header&2); //prv_alloc = prev_block's prv_alloc bit, alloc = 0
        prev_block->header = new_header;
        block_pointer = (char *)prev_block + (new_header&~3);
        struct sf_block *new_next_block = (struct sf_block *) block_pointer;
        new_next_block->prev_footer = new_header;
        malloc_remove_block(prev_block);
        //if wilderness block
        if(next_block->body.links.next == &sf_free_list_heads[NUM_FREE_LISTS-1]){
            malloc_remove_block(next_block);
            sf_free_list_heads[NUM_FREE_LISTS-1].body.links.next = prev_block;
            sf_free_list_heads[NUM_FREE_LISTS-1].body.links.prev = prev_block;
            prev_block->body.links.next = &sf_free_list_heads[NUM_FREE_LISTS-1];
            prev_block->body.links.prev = &sf_free_list_heads[NUM_FREE_LISTS-1];
        } else {
            malloc_split_insert(prev_block);
        }
        return;
    }
}

void sf_free(void *pp) {
    if(pp == NULL) //if pointer is NULL
        abort();
    if(((size_t)pp) % 64 != 0) //if pointer is not aligned to a 64-bit boundary
        abort();
    //getting pointer to block
    char *block_position = (char *)pp - 16;
    struct sf_block *arg_block = (struct sf_block *)block_position;
    if((arg_block->header&1) == 0) //if allocated bit in header is 0
        abort();
    if(pp < sf_mem_start()+112) //pointer address is before end of prologue(48+64 bytes from start)
        abort();
    if(pp > sf_mem_end()-8) //pointer address is after start of epilogue
        abort();
    //getting pointer to previous block for header
    if((arg_block->header&2) == 0 && (arg_block->prev_footer&1) != 0) //prv_alloc = 0 but alloc bit of previous block != 0
        abort();
    //free alloc bit
    arg_block->header = arg_block->header&~1;
    free_coalesce(arg_block);
    return;
}
/*HELPER METHOD - takes in a current block and resizes it down to a new size
*/
void realloc_split(struct sf_block *client_block, size_t rsize) {
    size_t temp_original_header = client_block->header;
    //creating new header for client_block
    client_block->header = rsize|1; //alloc bit set to 1
    client_block->header |= temp_original_header&2; //moving prv_alloc bit from old header
    //creating new block based off remainder
    char *heap_pointer = ((char *)client_block) + rsize;
    struct sf_block *new_block = (struct sf_block *) heap_pointer; //creates the new upper block
    new_block->header = (temp_original_header&~3)-rsize;
    new_block->header |= 3; //prev alloc bit = 1, alloc bit = 1, using hackerman trick
    new_block->prev_footer = client_block->header;
    //setting footer of new block
    heap_pointer = (char *)client_block + (temp_original_header&~3);
    struct sf_block *prev_footer_block = (struct sf_block *) heap_pointer;
    prev_footer_block->prev_footer = new_block->header;
    //use free to coalesce block for us
    sf_free((char *)new_block + 16); //200 IQ play
}

void *sf_realloc(void *pp, size_t rsize) {
    //same suite of pointer tests as free
    if(pp == NULL) //if pointer is NULL
        abort();
    if(((size_t)pp) % 64 != 0) //if pointer is not aligned to a 64-bit boundary
        abort();
    char *block_position = (char *)pp - 16;
    struct sf_block *client_block = (struct sf_block *)block_position;
    if((client_block->header&1) == 0) //if allocated bit in header is 0
        abort();
    if(pp < sf_mem_start()+112) //pointer address is before end of prologue(48+64 bytes from start)
        abort();
    if(pp > sf_mem_end()-8) //pointer address is after start of epilogue
        abort();
    if((client_block->header&2) == 0 && (client_block->prev_footer&1) != 0) //prv_alloc = 0 but alloc bit of previous block != 0
        abort();
    //if pointer is valid but rsize is 0
    if(rsize == 0){
        sf_free(pp);
        return NULL;
    }
    rsize += 8; //include header for blocksize
    //if reallocating to a larger block size
    if((client_block->header&~3) < rsize) {
        void *malloc_payload = sf_malloc(rsize);
        if(malloc_payload == NULL)
            return NULL;
        memcpy(malloc_payload, pp, (client_block->header&~3)-8);
        sf_free(pp);
        return malloc_payload;
    }
    //if reallocating to a smaller block size
    else {
        if(rsize%64) //if not already a multiple of 64
            rsize = (rsize|63) + 1; //round up to next multiple of 64
        //no splitting b/c splinter
        if((client_block->header&~3) - rsize < 64)
            return pp;
        //splitting b/c splinter
        realloc_split(client_block, rsize);
        return pp;
    }
    return NULL;
}

void *sf_memalign(size_t size, size_t align) {
    //checking align is at least minimum block size
    if(align < 64) {
        sf_errno = EINVAL;
        return NULL;
    }
    //checking that align is a multiple of 2
    size_t temp_align = align;
    if(temp_align % 2) {
        sf_errno = EINVAL;
        return NULL;
    }
    while(temp_align != 1) {
        if(temp_align % 2){
            sf_errno = EINVAL;
            return NULL;
        }
        temp_align /= 2;
    }
    if(size == 0)
        return NULL;
    //append size
    char *start_address = (char *)sf_malloc(size + 64 + align); //adding header size, min size, align
    //check if we can realloc to a slightly smaller size
    size_t blocksize_counter = 64; //satisifies minimum distance requirement
    struct sf_block *old_block = (struct sf_block *) (start_address - 16); //gets the block of the original
    size_t blocksize = old_block->header&~3;
    while(1){
        if((size_t)start_address % align == 0) { //alignment satisfied
            break; //skip loop
        }
        if(((size_t)start_address + blocksize_counter) % align == 0){ //new block address satisfies the alignment requirement
            if(blocksize - blocksize_counter > size){ //has sufficient space to hold size payload
                old_block->header = blocksize_counter|(old_block->header&2)|1; //copying over the prv_alloc bit of the old header
                //splitting from the beginning of the block
                //setting up new_block
                char *new_address = start_address + blocksize_counter - 16;
                struct sf_block *new_block = (struct sf_block *) new_address;
                new_block->header = (blocksize - blocksize_counter)|3; //alloc bit = 1, alloc bit = 1
                new_block->prev_footer = old_block->header;
                //setting up prev_footer of new_block
                new_address += new_block->header&~3;
                struct sf_block *prev_footer_block = (struct sf_block *)new_address;
                prev_footer_block->prev_footer = new_block->header;
                //freeing old block
                sf_free(start_address);
                //splitting new_block, first round size to multiple of 64
                if(size % 64)
                    size = (size|63) + 1;
                realloc_split(new_block, size);
                return (void *)(new_block->body.payload);
            }
        }
        blocksize_counter++;
    }
    //resize size
    if(size % 64)
        size = (size|63) + 1;
    realloc_split(old_block, size);
    return (void *)(old_block->body.payload);
}