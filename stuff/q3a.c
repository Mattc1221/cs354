#include <stdio.h>

void update(int *x) {
    int a = 22;
    a = *x;
    printf("%d ", a);
}

int main(void) {
    int a = 11;
    int *b = &a;
    printf("%d ", *b);
    update(b);
    printf("%d ", *b);
    return 0;
}
