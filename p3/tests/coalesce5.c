// check for coalesce free space (first chunk)
#include <assert.h>
#include <stdlib.h>
#include "heapAlloc.h"

int main() {
   assert(initHeap(4096) == 0);
   void * ptr[4];

   ptr[0] = allocHeap(800);
   assert(ptr[0] != NULL);

   ptr[1] = allocHeap(800);
   assert(ptr[1] != NULL);

   ptr[2] = allocHeap(800);
   assert(ptr[2] != NULL);

   ptr[3] = allocHeap(800);
   assert(ptr[3] != NULL);

   while (allocHeap(800) != NULL)
     ;

   assert(freeHeap(ptr[0]) == 0);
   assert(freeHeap(ptr[1]) == 0);

   ptr[2] = allocHeap(1600);
   assert(ptr[2] != NULL);

   exit(0);
}
