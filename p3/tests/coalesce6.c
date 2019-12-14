// check for coalesce free space (last chunk)
#include <assert.h>
#include <stdlib.h>
#include "heapAlloc.h"

#define HEADER (4)
#define SLACK (8)

int main() {
   assert(initHeap(4096) == 0);
   void * ptr[4];

   ptr[0] = allocHeap(884);
   assert(ptr[0] != NULL);

   ptr[1] = allocHeap(884);
   assert(ptr[1] != NULL);

   ptr[2] = allocHeap(884);
   assert(ptr[2] != NULL);

   ptr[3] = allocHeap(884);
   assert(ptr[3] != NULL);

   assert(allocHeap(884) == NULL);
   
   // last free chunk is at least this big
   int free = (4096 - (884 + HEADER) * 4) - SLACK;

   assert(freeHeap(ptr[3]) == 0);
   free += (884 + HEADER);
   ptr[3] = allocHeap(free - HEADER - 40);
   assert(ptr[3] != NULL);
   exit(0);
}
