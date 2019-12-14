// second allocation is too big to fit
#include <assert.h>
#include <stdlib.h>
#include "heapAlloc.h"

int main() {
   assert(initHeap(4096) == 0);
   assert(allocHeap(2048) != NULL);
   assert(allocHeap(2047) == NULL);
   exit(0);
}
