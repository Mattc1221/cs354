///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
//
///////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "heapAlloc.h"

/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct blockHeader {
        int size_status;
    /*
    * Size of the block is always a multiple of 8.
    * Size is stored in all block headers and free block footers.
    *
    * Status is stored only in headers using the two least significant bits.
    *   Bit0 => least significant bit, last bit
    *   Bit0 == 0 => free block
    *   Bit0 == 1 => allocated block
    *
    *   Bit1 => second last bit 
    *   Bit1 == 0 => previous block is free
    *   Bit1 == 1 => previous block is allocated
    * 
    * End Mark: 
    *  The end of the available memory is indicated using a size_status of 1.
    * 
    * Examples:
    * 
    * 1. Allocated block of size 24 bytes:
    *    Header:
    *      If the previous block is allocated, size_status should be 27
    *      If the previous block is free, size_status should be 25
    * 
    * 2. Free block of size 24 bytes:
    *    Header:
    *      If the previous block is allocated, size_status should be 26
    *      If the previous block is free, size_status should be 24
    *    Footer:
    *      size_status should be 24
    */
} blockHeader;         

/* Global variable - DO NOT CHANGE. It should always point to the first block,
 * i.e., the block at the lowest address.
 */

blockHeader *heapStart = NULL;
blockHeader *prev_alloced = NULL; // stores the most previously allocated memory block, to help with the NEXT-FIT placement policy
blockHeader *end_mark = NULL; // the end mark of the program 

/*
 * Argument size: requested size for the payload
 * Returns address of allocated block on success.
 * Returns NULL on failure.
 * This function should:
 * - Check size - Return NULL if not positive or if larger than heap space.
 * - Determine block size rounding up to a multiple of 8 and possibly adding padding as a result.
 * - Use NEXT-FIT PLACEMENT POLICY to chose a free block
 * - Use SPLITTING to divide the chosen free block into two if it is too large.
 * - Update header(s) and footer as needed.
 * Tips: Be careful with pointer arithmetic and scale factors.
 */
void* allocHeap(int size) {
    //TODO: Your code goes in here.
    
	// checking and modifying size
	if (size <= 0) return NULL; // checks for a valid size
	int actSize = size +4; // adds memory for block header
	if(actSize % 8 != 0) actSize += (8 - (actSize%8)); // Adds padding to follow double word addressibility

	// checking to make sure heap is initialized
	if(heapStart == NULL) return NULL;

	// block created for splitting. If splitting occurs this will be the new free block on the right side of the split
	blockHeader *newBlock = NULL;
	
	// checks if this is the first allocation
	if(prev_alloced == NULL) {
		prev_alloced = heapStart; // prev_alloc set to heapStart because its the first allocation
		end_mark = (blockHeader*)((void*)heapStart + heapStart->size_status - heapStart->size_status%8); // stores end_mark as a global vaiable
	}
	
	// finds the next spot to allocate memory based on NEXT_FIT placement policy
	blockHeader *current = prev_alloced;
	while(current->size_status%2 != 0 || current->size_status - current->size_status%8 < actSize){ // Increments the current one by one for next fit
		current = (blockHeader*)((void*)current +  current->size_status - current->size_status%8); // goes to next thingy
		if(current == prev_alloced){ return NULL; }// checks to see if a loop has been made
		if(current->size_status == 1){ // checks if end point is reached
		       	current = heapStart; // wrap around
			if(current == prev_alloced){ return NULL; }// checks to see if a loop has been made
		}
	}

	// gets the size and p bit of the NEXT-FIT 'free block'
	int curr_size = current->size_status - current->size_status%8; // gets the size of the mem block
	int pbit = current->size_status%8; // gets the p bit
	
	if(curr_size < actSize || current->size_status%2 == 1){ return NULL;}


	// Block for splitting the block
        if(actSize < curr_size){ // checks to make sure splitting is required
        	newBlock = (blockHeader*)((void*)current + actSize); // new block to help split if needed
                newBlock->size_status = curr_size + 2 - actSize;// updates the size status of the new free block
        }

	// updates the footer
	blockHeader *footer = (blockHeader*) ((char*)current + curr_size - 4); // finds the footer of the header
        if(actSize == curr_size){footer = NULL; }// gets rid of the footer if the whole heap start is taken up
        else{ footer->size_status = curr_size-actSize;} // updates the footer data
        
	// updates current
	current->size_status = actSize + pbit + 1; // updates the memory block of the new allocated memory
        
	// update prev_alloced
        prev_alloced = current; // tracks the newest allocated memory
	
    	return ((void*)current + 4); // returns pointer to payload of the new allocated memory block
}

/* 
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space. ---------------------------------------I think this is right, but also not sure
 * - Return -1 if ptr block is already freed.  --------------------------------------------I think this is write, but going to double check with TA
 * - USE IMMEDIATE COALESCING if one or both of the adjacent neighbors are free.
 * - Update header(s) and footer as needed.
 */                    
int freeHeap(void *ptr) {         
    //TODO: Your code goes in here.
    //
    //TODO: check it ptr is valid!!! - not done yet
    	if(ptr == NULL || (int)&ptr % 8 != 0 || ((blockHeader*)(ptr-4))->size_status%2 != 1) return -1; // checks if ptr is NULL, a multiple of 8, or already been freed
	if(ptr < (void*)heapStart || ptr >= (void*)end_mark) return -1; // checks to see if ptr is within the range of the heap memory
    	blockHeader *current = (blockHeader*)(ptr-4);
	int curr_size = current->size_status - current->size_status%8; // gets the size of the mem block
        int bits = current->size_status%8; // gets the bits
	
	// Checks to see if prev_alloced needs to be updated
        int changePrevAlloced = 0; // if 0 do not need to update prev_alloced later on, if 1 need to update prev_alloced later on
	if(current == prev_alloced) changePrevAlloced = 1; // actually does the checking
	
	// check to coalesce the fontside ;)
	blockHeader *next = (blockHeader*)((void*)current + curr_size); // gets the mem block in front of ptr
	if(next->size_status%2 == 0){ // Checks the a bit of the next block to test for coalescibility
		int next_size = next->size_status - next->size_status%8; // gets the size of the mem block
		curr_size += next_size; // sets up current size
		current->size_status = curr_size + bits; // updates current
		current->size_status -= current->size_status%2; // makes sure current is labled as unallocated
	}

	// coalesce the backside
	if(bits - bits%2 == 0){ // checks the p bit to see if the backside is coalescible 
		blockHeader *prev_footer = (blockHeader*)((void*)current - 4); // gets the footer of the previous memory block
		int prev_size = prev_footer->size_status; // gets the size of the previous memory block
		blockHeader *prev = (blockHeader*)((void*)current - prev_size); // gets the mem block in front of ptr
	 	int prev_bits = prev->size_status%8; // gets bits for prev
		curr_size += prev_size; // updates size
		current = prev; // changes currents position
		current->size_status = curr_size + prev_bits; // adds the p and a bits to current
		current->size_status -= current->size_status%2; // makes sure current is labled as unallocated
	}

	// fix the footer
	blockHeader *footer = (blockHeader*) ((char*)current + curr_size - 4); // finds the footer of the header
        footer->size_status = curr_size; // updates the footer size
        
	// Removing the a bit to mark it as free
	current->size_status -= current->size_status%2;

	// Change the p bit in the next block
	next = (blockHeader*)((void*)current + current->size_status - current->size_status%8); // gets the mem block in front of the new coalesced block
        int next_bits = next->size_status%8; // gets the bits
	if(next_bits - next_bits%2 != 0) next->size_status -= 2; // sets p bit to 0 

	// updates prev_alloced if need be
	if(changePrevAlloced) prev_alloced = current;

	return 0;
}

/*
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int initHeap(int sizeOfRegion) {         

    static int allocated_once = 0; //prevent multiple initHeap calls

    int pagesize;  // page size
    int padsize;   // size of padding when heap size not a multiple of page size
    int allocsize; // size of requested allocation including padding
    void* mmap_ptr; // pointer to memory mapped area
    int fd;

    blockHeader* endMark;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }
    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    allocsize = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignment and end mark
    allocsize -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heapStart = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    endMark = (blockHeader*)((void*)heapStart + allocsize);
    endMark->size_status = 1;

    // Set size in header
    heapStart->size_status = allocsize;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heapStart->size_status += 2;

    // Set the footer
    blockHeader *footer = (blockHeader*) ((char*)heapStart + allocsize - 4);
    footer->size_status = allocsize;
  
    return 0;
}
/* 
 * Function to be used for DEBUGGING to help you visualize your heap structure.
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void DumpMem() {  

    int counter;
    char status[5];
    char p_status[5];
    char *t_begin = NULL;
    char *t_end   = NULL;
    int t_size;

    blockHeader *current = heapStart;
    counter = 1;

    int used_size = 0;
    int free_size = 0;
    int is_used   = -1;

    fprintf(stdout, "************************************Block list***\
                    ********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, "-------------------------------------------------\
                    --------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "used");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "Free");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "used");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "Free");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%d\n", counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, "---------------------------------------------------\
                    ------------------------------\n");
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fprintf(stdout, "Total used size = %d\n", used_size);
    fprintf(stdout, "Total free size = %d\n", free_size);
    fprintf(stdout, "Total size = %d\n", used_size + free_size);
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fflush(stdout);

    return;  
}  
