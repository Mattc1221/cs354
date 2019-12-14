// a simple 8 byte allocation
#include <assert.h>
#include <stdlib.h>
#include "heapAlloc.h"

int main() {
    assert(initHeap(4096) == 0);
    void* ptr = allocHeap(8);
    assert(ptr != NULL);
    exit(0);
}
