#include <stdio.h>

void h(int (*fun)(int))
{
    printf("fun(42) = %d\n", fun(42));
}
