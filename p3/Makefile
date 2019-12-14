heapAlloc: heapAlloc.c heapAlloc.h
	gcc -g -c -Wall -m32 -fpic heapAlloc.c
	gcc -shared -Wall -m32 -o libheap.so heapAlloc.o

clean:
	rm -rf heapAlloc.o libheap.so
