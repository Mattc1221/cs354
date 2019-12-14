///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __heapAlloc_h
#define __heapAlloc_h

int   initHeap (int sizeOfRegion);
void* allocHeap(int size);
int   freeHeap (void *ptr);
void  dumpMem  ();

void* malloc(size_t size) {
    return NULL;
}

#endif // __heapAlloc_h__

