// many odd sized allocations checked for alignment
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include "heapAlloc.h"

int main() {
    assert(initHeap(4096) == 0);
    void* ptr[9];
    ptr[0] = allocHeap(1);
    ptr[1] = allocHeap(5);
    ptr[2] = allocHeap(14);
    ptr[3] = allocHeap(8);
    ptr[4] = allocHeap(1);
    ptr[5] = allocHeap(4);
    ptr[6] = allocHeap(9);
    ptr[7] = allocHeap(33);
    ptr[8] = allocHeap(55);
   
    for (int i = 0; i < 9; i++) {
        assert(ptr[i] != NULL);
    }

    for (int i = 0; i < 9; i++) {
        assert(((int)ptr[i]) % 8 == 0);
    }
    exit(0);
}
