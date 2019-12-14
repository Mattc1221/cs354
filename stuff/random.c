#include <stdio.h>

void f(int num1, int num2, int *ptr3) {
	int temp = *(ptr3 + num1);
	ptr3[num1] = ptr3[num2];
	*(ptr3 + num2) = temp;
}

int main(void) {
	int a = 3;
	int b = 0;
	int c[] = {13, 3, 21, 8, 2, 5};

	f(a, b, c+1);

	printf("%i,%i,%i,%i,%i,%i\n", c[0], c[1], c[2], c[3], c[4], c[5]);

	return 0;
}
