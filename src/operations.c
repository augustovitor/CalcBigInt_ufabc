#include "operations.h"
#include <stdlib.h>
#include <string.h>

/* helpers internos */

static BigInt *bigint_new(size_t len, int sign) {
    BigInt *r = malloc(sizeof(BigInt));
    if (!r) return NULL;
    r->data = calloc(len, sizeof(unsigned int));
    if (!r->data) { free(r); return NULL; }
    r->len = len;
    r->sign = sign;
    return r;
}

/* soma de valores absolutos: |a| + |b|, resultado sempre com sign=+1 */
static BigInt *bigint_add_abs(const BigInt *a, const BigInt *b) {
    size_t n = (a->len > b->len) ? a->len : b->len;
    BigInt *res = bigint_new(n + 1, 1);
    if (!res) return NULL;

    unsigned long long carry = 0;
    for (size_t i = 0; i < n; ++i) {
        unsigned long long av = (i < a->len) ? a->data[i] : 0;
        unsigned long long bv = (i < b->len) ? b->data[i] : 0;
        unsigned long long s = av + bv + carry;
        res->data[i] = (unsigned int)(s % BASE);
        carry = s / BASE;
    }
    if (carry) {
        res->data[n] = (unsigned int)carry;
        res->len = n + 1;
    } else {
        res->len = n;
    }
    bigint_normalize(res);
    return res;
}

/* subtração de valores absolutos: |a| - |b|, assumindo |a| >= |b| */
static BigInt *bigint_sub_abs(const BigInt *a, const BigInt *b) {
    BigInt *res = bigint_new(a->len, 1);
    if (!res) return NULL;

    long long carry = 0;
    for (size_t i = 0; i < a->len; ++i) {
        long long av = a->data[i];
        long long bv = (i < b->len) ? b->data[i] : 0;
        long long s = av - bv + carry;
        if (s < 0) {
            s += BASE;
            carry = -1;
        } else {
            carry = 0;
        }
        res->data[i] = (unsigned int)s;
    }
    bigint_normalize(res);
    return res;
}

/* soma geral com sinal */
BigInt *bigint_add(const BigInt *a, const BigInt *b) {
    if (!a || !b) return NULL;

    BigInt *res = NULL;

    if (a->sign == b->sign) {
        res = bigint_add_abs(a, b);
        if (res) res->sign = a->sign;
    } else {
        int cmp = bigint_cmpabs(a, b);
        if (cmp == 0) {
            /* resultado zero */
            res = bigint_new(1, 1);
            if (res) res->data[0] = 0;
        } else if (cmp > 0) {
            res = bigint_sub_abs(a, b);
            if (res) res->sign = a->sign;
        } else {
            res = bigint_sub_abs(b, a);
            if (res) res->sign = b->sign;
        }
    }

    if (res) bigint_normalize(res);
    return res;
}

/* subtração geral: a - b */
BigInt *bigint_sub(const BigInt *a, const BigInt *b) {
    if (!a || !b) return NULL;

    /* a - b = a + (-b) */
    BigInt tmp = *b;
    tmp.sign = -b->sign;
    return bigint_add(a, &tmp);
}

/* multiplicação: a * b */
BigInt *bigint_mul(const BigInt *a, const BigInt *b) {
    if (!a || !b) return NULL;

    BigInt *res = bigint_new(a->len + b->len, a->sign * b->sign);
    if (!res) return NULL;

    for (size_t i = 0; i < a->len; ++i) {
        unsigned long long carry = 0;
        for (size_t j = 0; j < b->len; ++j) {
            unsigned long long cur =
                res->data[i + j] +
                (unsigned long long)a->data[i] * (unsigned long long)b->data[j] +
                carry;
            res->data[i + j] = (unsigned int)(cur % BASE);
            carry = cur / BASE;
        }
        if (carry) {
            res->data[i + b->len] += (unsigned int)carry;
        }
    }

    bigint_normalize(res);
    if (res->len == 1 && res->data[0] == 0) {
        res->sign = 1;
    }
    return res;
}

/* cópia de valor absoluto */
static BigInt *bigint_abs_copy(const BigInt *x) {
    BigInt *r = bigint_new(x->len, 1);
    if (!r) return NULL;
    memcpy(r->data, x->data, x->len * sizeof(unsigned int));
    bigint_normalize(r);
    return r;
}

/* cria BigInt a partir de inteiro pequeno (0 ou 1, etc.) */
static BigInt *bigint_from_uint(unsigned int v) {
    BigInt *r = bigint_new(1, 1);
    if (!r) return NULL;
    r->data[0] = v;
    bigint_normalize(r);
    return r;
}

/* Divisão inteira e resto: a / b = q, a % b = r
   Implementação simples (repetidas subtrações) - correta, mas lenta para números enormes. */
void bigint_divmod(const BigInt *a, const BigInt *b, BigInt **q, BigInt **r) {
    if (q) *q = NULL;
    if (r) *r = NULL;
    if (!a || !b) return;

    /* divisor zero -> erro simples */
    if (b->len == 1 && b->data[0] == 0) {
        return;
    }

    int cmp = bigint_cmpabs(a, b);
    if (cmp < 0) {
        /* |a| < |b| -> quociente = 0, resto = a */
        if (q) *q = bigint_from_uint(0);
        if (r) {
            *r = bigint_abs_copy(a);
            if (*r) (*r)->sign = a->sign;
        }
        return;
    }

    BigInt *dividend = bigint_abs_copy(a);
    BigInt *divisor  = bigint_abs_copy(b);
    BigInt *one      = bigint_from_uint(1);
    BigInt *quotient = bigint_from_uint(0);

    if (!dividend || !divisor || !one || !quotient) {
        bigint_free(dividend);
        bigint_free(divisor);
        bigint_free(one);
        bigint_free(quotient);
        return;
    }

    while (bigint_cmpabs(dividend, divisor) >= 0) {
        BigInt *tmp = bigint_sub(dividend, divisor);
        bigint_free(dividend);
        dividend = tmp;

        BigInt *tmpq = bigint_add(quotient, one);
        bigint_free(quotient);
        quotient = tmpq;
    }

    bigint_normalize(quotient);
    bigint_normalize(dividend);

    /* ajusta sinais */
    if (quotient->len == 1 && quotient->data[0] == 0)
        quotient->sign = 1;
    else
        quotient->sign = a->sign * b->sign;

    if (dividend->len == 1 && dividend->data[0] == 0)
        dividend->sign = 1;
    else
        dividend->sign = a->sign;

    if (q) *q = quotient; else bigint_free(quotient);
    if (r) *r = dividend; else bigint_free(dividend);

    bigint_free(divisor);
    bigint_free(one);
}
