#include "operations.h"
#include <stdlib.h>
#include <string.h>

BigInt *bigint_add(const BigInt *a, const BigInt *b) {
    size_t n = (a->len > b->len) ? a->len : b->len;
    BigInt *res = malloc(sizeof(BigInt));
    res->data = calloc(n + 1, sizeof(unsigned int));
    unsigned long long carry = 0;
    for (size_t i = 0; i < n; ++i) {
        unsigned long long av = (i < a->len) ? a->data[i] : 0;
        unsigned long long bv = (i < b->len) ? b->data[i] : 0;
        unsigned long long s = av + bv + carry;
        res->data[i] = (unsigned int)(s % BASE);
        carry = s / BASE;
    }
    if (carry) { res->data[n] = (unsigned int)carry; res->len = n + 1; }
    else res->len = n;
    res->sign = 1;
    return res;
}

BigInt *bigint_sub(const BigInt *a, const BigInt *b) { return NULL; }
BigInt *bigint_mul(const BigInt *a, const BigInt *b) { return NULL; }
void bigint_divmod(const BigInt *a, const BigInt *b, BigInt **q, BigInt **r) { *q = NULL; *r = NULL; }