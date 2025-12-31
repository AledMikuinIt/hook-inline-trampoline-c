#include <stdio.h>

int main() {
    int a = 5, b = 7;
    int result;

    __asm__ (
        "movl %[x], %%eax\n\t"
        "addl %[y], %%eax\n\t"
        "movl %%eax, %[res]"
        : [res] "=r"(result)
        : [x] "r"(a), [y] "r"(b)
        : "%eax"
    );

    printf("Le résultat de %d + %d = %d\n", a, b, result);
    return 0;
}
