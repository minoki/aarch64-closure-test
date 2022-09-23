#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "lib.h"

void greet(const char *message, int a, int b)
{
    printf("%s: %d, %d\n", message, a, b);
}

void (*make_greet(const char *message))(int, int)
{
    return (void (*)(int, int))make_closure((FP)greet, (void *)message, 2, (const enum arg_type []){I32, I32});
}

double manyargs(const char *message, int a, int b, int c, int d, int e, int f, char g, short h, double i, float j, double k, long long l)
{
    printf("manyargs(%s, %d, %d, %d, %d, %d, %d, %c, %d, %g, %g, %g, %lld)\n", message, a, b, c, d, e, f, g, h, i, j, k, l);
    return k;
}
typedef double (*manyargs_type)(int, int, int, int, int, int, char, short, double, float, double, long long);

double manyargs2(const char *message, int a, double b, long long c, float d, char e, float f, int g, double h, char i, double j, long long k, float l, short m, float n, char o, double p, short q, float r)
{
    printf("manyargs2(%s, %d, %g, %lld, %g, %c, %g, %d, %g, %c, %g, %lld, %g, %d, %g, %c, %g, %d, %g)\n", message, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r);
    return j;
}
typedef double (*manyargs2_type)(int a, double b, long long c, float d, char e, float f, int g, double h, char i, double j, long long k, float l, short m, float n, char o, double p, short q, float r);

int main(void)
{
    {
        void (*greet1)(int, int) = make_greet("Hello world!");
        greet1(32, 48);
        greet1(0, 1);
        void (*greet2)(int, int) = make_greet("Goodbye world!");
        greet2(77, 99);
        greet1(0, -1);
    }
    {
        manyargs_type manyargsA = (manyargs_type)make_closure((FP)manyargs, "A", 12, (const enum arg_type [12]){I32, I32, I32, I32, I32, I32, I8, I16, F64, F32, F64, I64});
        manyargsA(0, 1, 2, 3, 4, 5, 'x', 42, 3.14, -0.1f, 2.71, -1LL);
        manyargsA(0, 10, 20, 30, 40, 50, 'y', 420, -3.14, 0.1f, -2.71, 1LL);
        manyargs_type manyargsB = (manyargs_type)make_closure((FP)manyargs, "B", 12, (const enum arg_type [12]){I32, I32, I32, I32, I32, I32, I8, I16, F64, F32, F64, I64});
        manyargsB(0, 1, 2, 3, 4, 5, 'x', 42, 3.14, -0.1f, 2.71, -1LL);
        manyargsB(0, 10, 20, 30, 40, 50, 'y', 420, -3.14, 0.1f, -2.71, 1LL);
    }
    {
        manyargs2_type manyargsA = (manyargs2_type)make_closure((FP)manyargs2, "Hey", 18, (const enum arg_type [18]){I32, F64, I64, F32, I8, F32, I32, F64, I8, F64, I64, F32, I16, F32, I8, F64, I16, F32});
        manyargsA(0, 1.0, -2, 3.14f, 'E', -3.333f, -27, 7.77, 'x', 6.28, LLONG_MAX, 1.1f, SHRT_MAX, -0.0f, 'z', 2.9, 777, 777.7f);
    }
}
