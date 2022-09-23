#include <stdio.h>
#include <Block.h>

void h(int (^g)(int)) {
    printf("g(42) = %d\n", g(42));
}

int (^f(const int x))(int) {
    return Block_copy(^int (int y) {
            return x + y;
        });
}

int main(void) {
    int (^g1)(int) = f(2);
    int (^g2)(int) = f(-7);
    h(g1);
    h(g2);
    printf("%zu\n", sizeof(int (^)(int)));
}
