#include <stdio.h>
int main(void) {
    int opt = 0;
    long a, b;
    while (1) {
        printf("Digite dois valores inteiros:\n");
        if (scanf("%ld %ld", &a, &b) != 2) break;
        printf("Digite uma opcao entre 1 e 5:\n");
        printf("1) Soma\n2) Subtracao\n3) Multiplicacao\n4) Divisao\n5) Sair\n");
        if (scanf("%d", &opt) != 1) break;
        if (opt == 5) break;
        if (opt == 1) printf("Soma e %ld\n", a + b);
        else if (opt == 2) printf("Subtracao e %ld\n", a - b);
        else if (opt == 3) printf("Multiplicacao e %ld\n", a * b);
        else if (opt == 4) printf("Divisao e %ld\n", a / b);
    }
    return 0;
}