// many odd sized allocations and interspersed frees
#include <assert.h>
#include <stdlib.h>
#include "heapAlloc.h"

int main() {
   assert(initHeap(4096) == 0);
   void * ptr[9];
   ptr[0] = allocHeap(1);
   ptr[1] = (allocHeap(5));
   ptr[2] = (allocHeap(14));
   ptr[3] = (allocHeap(8));
   assert(ptr[0] != NULL);
   assert(ptr[1] != NULL);
   assert(ptr[2] != NULL);
   assert(ptr[3] != NULL);
   
   assert(freeHeap(ptr[1]) == 0);
   assert(freeHeap(ptr[0]) == 0);
   assert(freeHeap(ptr[3]) == 0);
   
   ptr[4] = (allocHeap(1));
   ptr[5] = (allocHeap(4));
   assert(ptr[4] != NULL);
   assert(ptr[5] != NULL);
   assert(freeHeap(ptr[5]) == 0);
   
   ptr[6] = (allocHeap(9));
   ptr[7] = (allocHeap(33));
   assert(ptr[6] != NULL);
   assert(ptr[7] != NULL);
   
   assert(freeHeap(ptr[4]) == 0);

   ptr[8] = (allocHeap(55));
   assert(ptr[8] != NULL);

   assert(freeHeap(ptr[2]) == 0);
   assert(freeHeap(ptr[7]) == 0);
   assert(freeHeap(ptr[8]) == 0);
   assert(freeHeap(ptr[6]) == 0);

   exit(0);
}
