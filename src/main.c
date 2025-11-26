#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bigint.h"
#include "operations.h"
#include "io.h"

int main(void) {
    char line[64];

    while (1) {
        printf("=== CalcBigInt ===\n");
        printf("1) Soma\n");
        printf("2) Subtracao\n");
        printf("3) Multiplicacao\n");
        printf("4) Divisao inteira\n");
        printf("5) Modulo (resto)\n");
        printf("6) Sair\n");
        printf("Escolha: ");

        if (!fgets(line, sizeof(line), stdin)) break;

        int op = 0;
        if (sscanf(line, "%d", &op) != 1) {
            printf("Opcao invalida.\n\n");
            continue;
        }

        if (op == 6) {
            printf("Saindo...\n");
            break;
        }

        if (op < 1 || op > 5) {
            printf("Opcao invalida.\n\n");
            continue;
        }

        printf("Digite o primeiro numero:\n");
        BigInt *a = read_bigint_stdin();
        if (!a) { printf("Erro na leitura.\n"); continue; }

        printf("Digite o segundo numero:\n");
        BigInt *b = read_bigint_stdin();
        if (!b) { printf("Erro na leitura.\n"); bigint_free(a); continue; }

        if (op == 1 || op == 2 || op == 3) {
            BigInt *res = NULL;
            if (op == 1) res = bigint_add(a, b);
            else if (op == 2) res = bigint_sub(a, b);
            else if (op == 3) res = bigint_mul(a, b);

            if (!res) {
                printf("Erro na operacao.\n");
            } else {
                char *s = bigint_to_string(res);
                if (s) {
                    printf("Resultado: %s\n", s);
                    free(s);
                }
                bigint_free(res);
            }
        } else if (op == 4 || op == 5) {
            BigInt *q = NULL;
            BigInt *r = NULL;
            bigint_divmod(a, b, &q, &r);

            if (!q && !r) {
                printf("Erro na divisao (talvez divisor zero).\n");
            } else {
                if (op == 4 && q) {
                    char *sq = bigint_to_string(q);
                    if (sq) {
                        printf("Quociente: %s\n", sq);
                        free(sq);
                    }
                }
                if (op == 5 && r) {
                    char *sr = bigint_to_string(r);
                    if (sr) {
                        printf("Resto: %s\n", sr);
                        free(sr);
                    }
                }
            }

            if (q) bigint_free(q);
            if (r) bigint_free(r);
        }

        bigint_free(a);
        bigint_free(b);

        printf("\n");
    }

    return 0;
}
