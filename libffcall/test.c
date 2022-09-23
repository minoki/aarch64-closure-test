#include <stdio.h>
#include <callback.h> // libffcall

void h(int (*fun)(int))
{
    printf("fun(42) = %d\n", fun(42));
}

void g(void *context, va_alist alist)
{
    int x = *(int *)context;
    va_start_int(alist);
    int y = va_arg_int(alist);
    va_return_int(alist, x + y);
}

int main()
{
    int x = -5;
    int (*fun)(int) = (int (*)(int))alloc_callback(&g, &x);
    printf("fun(77) = %d\n", fun(77));
    h(fun);
    free_callback(fun);
}
