#ifndef BIGINT_H
#define BIGINT_H

#include <stddef.h>

typedef struct {
    int sign;              /* +1 ou -1 */
    size_t len;            /* quantidade de blocos usados em data */
    unsigned int *data;    /* blocos em base BASE, little-endian (data[0] = menos significativo) */
} BigInt;

#define BASE 1000000000u   /* 1e9 */
#define BASE_DIGITS 9

/* Criação / destruição */
BigInt *bigint_from_string(const char *s);
char *bigint_to_string(const BigInt *a);
void bigint_free(BigInt *a);

/* Comparação de valor absoluto: -1 se |a| < |b|, 0 se igual, 1 se |a| > |b| */
int bigint_cmpabs(const BigInt *a, const BigInt *b);

/* Normalização: remove blocos zero no topo e ajusta sinal se for zero */
void bigint_normalize(BigInt *a);

#endif
