#include <stdio.h>
#include <stdlib.h>
#include "bigint.h"
#include "operations.h"
#include "io.h"

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    while (1) {
        printf("=== CalcBigInt ===\n");
        printf("1) Soma\n2) Subtracao\n3) Multiplicacao\n4) Divisao\n5) Sair\nEscolha: ");
        int op = 0;
        if (scanf("%d", &op) != 1) break;
        if (op == 5) break;
        printf("Digite o primeiro numero:\n");
        BigInt *a = read_bigint_stdin();
        printf("Digite o segundo numero:\n");
        BigInt *b = read_bigint_stdin();
        BigInt *res = NULL;
        if (op == 1) res = bigint_add(a, b);
        char *s = res ? bigint_to_string(res) : NULL;
        printf("Resultado: %s\n", s ? s : "(nao implementado ainda)");
        free(s);
        bigint_free(a); bigint_free(b); bigint_free(res);
        int c; while ((c = getchar()) != '\n' && c != EOF);
    }
    return 0;
}