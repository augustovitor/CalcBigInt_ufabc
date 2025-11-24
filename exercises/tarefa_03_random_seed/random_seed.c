#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    unsigned long seed; int n;
    printf("Seed: ");
    if (scanf("%lu", &seed) != 1) return 0;
    printf("Numero de digitos: ");
    if (scanf("%d", &n) != 1) return 0;
    srand((unsigned int)seed);
    if (n <= 0) return 0;
    char *s = malloc(n+1);
    for (int i = 0; i < n; ++i) {
        int d = rand() % 10;
        if (i == 0 && d == 0) d = 1 + rand() % 9;
        s[i] = '0' + d;
    }
    s[n] = '\0';
    printf("%s\n", s);
    free(s);
    return 0;
}