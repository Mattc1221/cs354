// a few allocations in multiples of 4 bytes
#include <assert.h>
#include <stdlib.h>
#include "heapAlloc.h"

int main() {
   assert(initHeap(4096) == 0);
   assert(allocHeap(4) != NULL);
   assert(allocHeap(8) != NULL);
   assert(allocHeap(16) != NULL);
   assert(allocHeap(24) != NULL);
   exit(0);
}
