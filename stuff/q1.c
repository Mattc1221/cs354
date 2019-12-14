#include <stdio.h>

int main(void){
	int i = 11;
	int *ptr1 = &i;
	int **ptr2 = &ptr1;
	int ***ptr3 = &ptr2;

	printf("%p, %p, %p, %p\n", ptr1, ptr2, ptr3, &ptr3);

	return 0;
}
