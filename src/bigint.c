#include "bigint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

BigInt *bigint_from_string(const char *s) {
    if (!s) return NULL;

    while (*s && isspace((unsigned char)*s)) s++;

    int sign = 1;
    if (*s == '+' || *s == '-') {
        if (*s == '-') sign = -1;
        s++;
    }

    while (*s == '0') s++;  /* tira zeros à esquerda */

    size_t slen = strlen(s);
    if (slen == 0) {
        /* é zero */
        BigInt *z = malloc(sizeof(BigInt));
        if (!z) return NULL;
        z->sign = 1;
        z->len = 1;
        z->data = calloc(1, sizeof(unsigned int));
        if (!z->data) { free(z); return NULL; }
        z->data[0] = 0;
        return z;
    }

    size_t nblocks = (slen + BASE_DIGITS - 1) / BASE_DIGITS;
    BigInt *res = malloc(sizeof(BigInt));
    if (!res) return NULL;
    res->sign = sign;
    res->len = nblocks;
    res->data = calloc(nblocks, sizeof(unsigned int));
    if (!res->data) { free(res); return NULL; }

    size_t idx = 0;
    for (ssize_t i = (ssize_t)slen; i > 0; i -= BASE_DIGITS) {
        ssize_t start = i - BASE_DIGITS;
        if (start < 0) start = 0;
        char buf[BASE_DIGITS + 1];
        size_t l = (size_t)(i - start);
        memcpy(buf, s + start, l);
        buf[l] = '\0';
        res->data[idx++] = (unsigned int)atoi(buf);
    }

    return res;
}

char *bigint_to_string(const BigInt *a) {
    if (!a) return NULL;

    if (a->len == 1 && a->data[0] == 0) {
        char *s = malloc(2);
        if (!s) return NULL;
        s[0] = '0';
        s[1] = '\0';
        return s;
    }

    size_t estimate = a->len * BASE_DIGITS + 2;
    char *out = malloc(estimate);
    if (!out) return NULL;

    char *p = out;
    if (a->sign < 0) *p++ = '-';

    int first = 1;
    for (size_t i = a->len; i-- > 0;) {
        if (first) {
            p += sprintf(p, "%u", a->data[i]);
            first = 0;
        } else {
            p += sprintf(p, "%09u", a->data[i]);
        }
    }
    *p = '\0';
    return out;
}

void bigint_free(BigInt *a) {
    if (!a) return;
    free(a->data);
    free(a);
}

int bigint_cmpabs(const BigInt *a, const BigInt *b) {
    if (a->len < b->len) return -1;
    if (a->len > b->len) return 1;
    for (size_t i = a->len; i-- > 0;) {
        if (a->data[i] < b->data[i]) return -1;
        if (a->data[i] > b->data[i]) return 1;
    }
    return 0;
}

void bigint_normalize(BigInt *a) {
    if (!a) return;
    while (a->len > 1 && a->data[a->len - 1] == 0) {
        a->len--;
    }
    if (a->len == 1 && a->data[0] == 0) {
        a->sign = 1; /* zero é positivo por convenção */
    }
}
