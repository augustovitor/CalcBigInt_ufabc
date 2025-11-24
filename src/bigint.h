#ifndef BIGINT_H
#define BIGINT_H

#include <stddef.h>

typedef struct {
    int sign;
    size_t len;
    unsigned int *data;
} BigInt;

#define BASE 1000000000u
#define BASE_DIGITS 9

BigInt *bigint_from_string(const char *s);
char *bigint_to_string(const BigInt *a);
void bigint_free(BigInt *a);
int bigint_cmpabs(const BigInt *a, const BigInt *b);

#endif