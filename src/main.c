/* main.c - versão monolítica com MDC e divisão longa.
 *
 * Mantém a base BASE = 1e9 (int de 32 bits) e usa unsigned long long para operações 64-bit.
 *
 * Inclui:
 * - BigInt básico (criação, string <-> bigint, free)
 * - operações: add, sub, mul, divmod (divisão longa rápida)
 * - gcd (MDC) via algoritmo de Euclides usando divmod
 * - IO (leitura de stdin)
 * - menu com opção MDC
 *
 * Salve em src/main.c se seu Makefile está no diretório pai.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    int sign;              /* +1 ou -1 */
    size_t len;            /* quantidade de blocos usados em data */
    unsigned int *data;    /* blocos em base BASE, little-endian (data[0] = menos significativo) */
} BigInt;

#define BASE 1000000000u   /* 1e9 */
#define BASE_DIGITS 9

/* Protos */
BigInt *bigint_from_string(const char *s);
char *bigint_to_string(const BigInt *a);
void bigint_free(BigInt *a);
void bigint_normalize(BigInt *a);
int bigint_cmpabs(const BigInt *a, const BigInt *b);

BigInt *bigint_add(const BigInt *a, const BigInt *b);
BigInt *bigint_sub(const BigInt *a, const BigInt *b);
BigInt *bigint_mul(const BigInt *a, const BigInt *b);
void bigint_divmod(const BigInt *a, const BigInt *b, BigInt **q, BigInt **r);

BigInt *read_bigint_stdin(void);

/* ------------------ Implementação ------------------ */

BigInt *bigint_from_string(const char *s) {
    if (!s) return NULL;

    while (*s && isspace((unsigned char)*s)) s++;

    int sign = 1;
    if (*s == '+' || *s == '-') {
        if (*s == '-') sign = -1;
        s++;
    }

    while (*s == '0') s++;  /* remove leading zeros */

    size_t slen = strlen(s);
    if (slen == 0) {
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

    bigint_normalize(res);
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

void bigint_normalize(BigInt *a) {
    if (!a) return;
    while (a->len > 1 && a->data[a->len - 1] == 0) a->len--;
    if (a->len == 1 && a->data[0] == 0) a->sign = 1;
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

/* auxiliares */
static BigInt *bigint_new(size_t len, int sign) {
    BigInt *r = malloc(sizeof(BigInt));
    if (!r) return NULL;
    r->data = calloc(len, sizeof(unsigned int));
    if (!r->data) { free(r); return NULL; }
    r->len = len;
    r->sign = sign;
    return r;
}

static BigInt *bigint_abs_copy(const BigInt *x) {
    BigInt *r = bigint_new(x->len, 1);
    if (!r) return NULL;
    memcpy(r->data, x->data, x->len * sizeof(unsigned int));
    bigint_normalize(r);
    return r;
}

static BigInt *bigint_from_uint(unsigned int v) {
    BigInt *r = bigint_new(1, 1);
    if (!r) return NULL;
    r->data[0] = v;
    bigint_normalize(r);
    return r;
}

/* add/sub abs helpers */
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

/* add/sub public */
BigInt *bigint_add(const BigInt *a, const BigInt *b) {
    if (!a || !b) return NULL;
    BigInt *res = NULL;
    if (a->sign == b->sign) {
        res = bigint_add_abs(a, b);
        if (res) res->sign = a->sign;
    } else {
        int cmp = bigint_cmpabs(a, b);
        if (cmp == 0) {
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

BigInt *bigint_sub(const BigInt *a, const BigInt *b) {
    if (!a || !b) return NULL;
    BigInt tmp = *b;
    tmp.sign = -b->sign;
    return bigint_add(a, &tmp);
}

/* Multiplicação padrão O(n*m) */
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
    if (res->len == 1 && res->data[0] == 0) res->sign = 1;
    return res;
}

/* multiplicar BigInt por small (unsigned int) -> novo BigInt */
static BigInt *bigint_mul_small(const BigInt *a, unsigned int m) {
    if (!a) return NULL;
    if (m == 0) return bigint_from_uint(0);
    BigInt *res = bigint_new(a->len + 1, a->sign);
    if (!res) return NULL;
    unsigned long long carry = 0;
    for (size_t i = 0; i < a->len; ++i) {
        unsigned long long cur = (unsigned long long)a->data[i] * m + carry;
        res->data[i] = (unsigned int)(cur % BASE);
        carry = cur / BASE;
    }
    if (carry) {
        res->data[a->len] = (unsigned int)carry;
        res->len = a->len + 1;
    } else {
        res->len = a->len;
    }
    bigint_normalize(res);
    return res;
}

/* divisão por small (unsigned int), retorna quociente novo e resto via ponteiro */
static BigInt *bigint_divmod_small(const BigInt *a, unsigned int m, unsigned int *rem_out) {
    if (!a || m == 0) return NULL;
    BigInt *res = bigint_new(a->len, a->sign);
    if (!res) return NULL;
    unsigned long long carry = 0;
    for (size_t i = a->len; i-- > 0;) {
        unsigned long long cur = a->data[i] + carry * BASE;
        unsigned int q = (unsigned int)(cur / m);
        carry = cur % m;
        res->data[i] = q;
    }
    if (rem_out) *rem_out = (unsigned int)carry;
    bigint_normalize(res);
    return res;
}

/* shift-left by k limbs (multiply by BASE^k) */
static BigInt *bigint_shift_left(const BigInt *a, size_t k) {
    if (!a) return NULL;
    if ((a->len == 1 && a->data[0] == 0) || k == 0) return bigint_abs_copy(a);
    BigInt *r = bigint_new(a->len + k, a->sign);
    if (!r) return NULL;
    for (size_t i = 0; i < a->len; ++i) r->data[i + k] = a->data[i];
    bigint_normalize(r);
    return r;
}

/* compare absolute, but allow different lengths (already have bigint_cmpabs) */

/* Subtract b * mult shifted by shift from a_inplace: a -= b * mult * BASE^shift
   Returns borrow flag (0 if no borrow, 1 if result negative happened -- caller must handle) */
static int bigint_sub_mul_shift(BigInt *a, const BigInt *b, unsigned int mult, size_t shift) {
    unsigned long long carry = 0;
    long long borrow = 0;
    size_t bi_len = b->len;
    for (size_t i = 0; i < bi_len || carry; ++i) {
        unsigned long long bv = 0;
        if (i < bi_len) bv = (unsigned long long)b->data[i] * mult;
        unsigned long long sub = bv + carry;
        carry = sub / BASE;
        unsigned int sub_low = (unsigned int)(sub % BASE);

        size_t ai = i + shift;
        long long cur = (long long)a->data[ai] - (long long)sub_low + borrow;
        if (cur < 0) {
            cur += BASE;
            borrow = -1;
        } else borrow = 0;
        a->data[ai] = (unsigned int)cur;
    }
    /* propagate borrow */
    size_t ai = bi_len + shift;
    while (borrow && ai < a->len) {
        long long cur = (long long)a->data[ai] + borrow;
        if (cur < 0) {
            cur += BASE;
            borrow = -1;
        } else borrow = 0;
        a->data[ai] = (unsigned int)cur;
        ai++;
    }
    return borrow ? 1 : 0;
}

/* add back b * mult shifted by shift to a: a += b * mult * BASE^shift */
static void bigint_add_mul_shift(BigInt *a, const BigInt *b, unsigned int mult, size_t shift) {
    unsigned long long carry = 0;
    size_t bi_len = b->len;
    for (size_t i = 0; i < bi_len || carry; ++i) {
        unsigned long long bv = 0;
        if (i < bi_len) bv = (unsigned long long)b->data[i] * mult;
        unsigned long long add = bv + carry;
        carry = add / BASE;
        unsigned int add_low = (unsigned int)(add % BASE);

        size_t ai = i + shift;
        unsigned long long cur = (unsigned long long)a->data[ai] + add_low;
        a->data[ai] = (unsigned int)(cur % BASE);
        unsigned long long c2 = cur / BASE;

        /* propagate carry + c2 */
        size_t j = ai + 1;
        unsigned long long carry2 = c2;
        while (carry2) {
            unsigned long long cur2 = (unsigned long long)a->data[j] + carry2;
            a->data[j] = (unsigned int)(cur2 % BASE);
            carry2 = cur2 / BASE;
            j++;
        }
    }
}

/* Divisão longa (Knuth-like) */
void bigint_divmod(const BigInt *a, const BigInt *b, BigInt **q_out, BigInt **r_out) {
    if (q_out) *q_out = NULL;
    if (r_out) *r_out = NULL;
    if (!a || !b) return;

    /* divisor zero -> return (q=NULL,r=NULL) to signal error */
    if (b->len == 1 && b->data[0] == 0) return;

    /* |a| < |b| -> q = 0, r = a */
    if (bigint_cmpabs(a, b) < 0) {
        if (q_out) *q_out = bigint_from_uint(0);
        if (r_out) {
            *r_out = bigint_abs_copy(a);
            if (*r_out) (*r_out)->sign = a->sign;
        }
        return;
    }

    /* Fast path: divisor has single limb -> use divmod small */
    if (b->len == 1) {
        unsigned int rem = 0;
        BigInt *q = bigint_divmod_small(a, b->data[0], &rem);
        if (q) q->sign = a->sign * b->sign;
        BigInt *r = bigint_from_uint(rem);
        if (r) r->sign = a->sign;
        if (q_out) *q_out = q; else bigint_free(q);
        if (r_out) *r_out = r; else bigint_free(r);
        return;
    }

    /* General long division: following standard algorithm.
       We'll work on absolute values, then set signs at the end. */

    size_t n = b->len;
    size_t m = a->len - n; /* result length will be m+1 */

    /* make copies of |a| with one extra limb at end for convenience */
    unsigned int *u = calloc(a->len + 1, sizeof(unsigned int));
    if (!u) return;
    for (size_t i = 0; i < a->len; ++i) u[i] = a->data[i];
    u[a->len] = 0;

    unsigned int *v = calloc(n, sizeof(unsigned int));
    if (!v) { free(u); return; }
    for (size_t i = 0; i < n; ++i) v[i] = b->data[i];

    /* normalization: choose factor so that highest digit of v >= BASE/2 */
    unsigned int v_high = v[n-1];
    unsigned int d = (unsigned int)(BASE / ((unsigned long long)v_high + 1ULL)); /* d >= 1 */
    if (d > 1) {
        /* multiply u and v by d (small) */
        unsigned long long carry = 0;
        for (size_t i = 0; i < a->len + 1; ++i) {
            unsigned long long cur = (unsigned long long)u[i] * d + carry;
            u[i] = (unsigned int)(cur % BASE);
            carry = cur / BASE;
        }
        /* ignore overflow (u has a[len] allocated) */

        carry = 0;
        for (size_t i = 0; i < n; ++i) {
            unsigned long long cur = (unsigned long long)v[i] * d + carry;
            v[i] = (unsigned int)(cur % BASE);
            carry = cur / BASE;
        }
        /* carry should be zero or less than BASE */
    }

    BigInt *q = bigint_new(m + 1, 1);
    if (!q) { free(u); free(v); return; }

    for (ssize_t j = (ssize_t)m; j >= 0; --j) {
        /* estimate qhat = (u[j+n]*BASE + u[j+n-1]) / v[n-1] */
        unsigned long long uj_n = u[j + n];
        unsigned long long uj_n1 = u[j + n - 1];
        unsigned long long numerator = uj_n * BASE + uj_n1;
        unsigned long long qhat = numerator / v[n - 1];
        unsigned long long rhat = numerator % v[n - 1];

        if (qhat >= BASE) qhat = BASE - 1;

        /* refine qhat: while qhat * v[n-2] > BASE * rhat + u[j+n-2] */
        while (qhat > 0) {
            unsigned long long left = qhat * (unsigned long long)v[n - 2];
            unsigned long long right = rhat * BASE + u[j + n - 2];
            if (left <= right) break;
            qhat--;
            rhat += v[n - 1];
            if (rhat >= BASE) break; /* safety */
        }

        /* multiply v by qhat and subtract from u starting at position j */
        unsigned long long carry = 0;
        long long borrow = 0;
        for (size_t i = 0; i < n; ++i) {
            unsigned long long p = (unsigned long long)v[i] * qhat + carry;
            carry = p / BASE;
            unsigned int p_low = (unsigned int)(p % BASE);

            long long cur = (long long)u[i + j] - (long long)p_low + borrow;
            if (cur < 0) {
                cur += BASE;
                borrow = -1;
            } else borrow = 0;
            u[i + j] = (unsigned int)cur;
        }
        /* subtract carry */
        long long cur = (long long)u[j + n] - (long long)carry + borrow;
        if (cur < 0) {
            /* qhat was too big; decrement qhat and add back v */
            qhat--;
            unsigned long long carry2 = 0;
            for (size_t i = 0; i < n; ++i) {
                unsigned long long cur2 = (unsigned long long)u[i + j] + v[i] + carry2;
                u[i + j] = (unsigned int)(cur2 % BASE);
                carry2 = cur2 / BASE;
            }
            u[j + n] = (unsigned int)((unsigned long long)u[j + n] + carry2);
            /* Note: possible multiple corrections rarely needed; loop not added for simplicity */
        } else {
            u[j + n] = (unsigned int)cur;
        }

        q->data[j] = (unsigned int)qhat;
    }

    /* quotient is q (need to normalize length) */
    bigint_normalize(q);

    /* remainder: u (need to divide by d if normalized) */
    BigInt *r = bigint_new(n, 1);
    if (!r) { bigint_free(q); free(u); free(v); return; }
    for (size_t i = 0; i < n; ++i) r->data[i] = u[i];
    r->len = n;
    bigint_normalize(r);

    /* if normalization factor d > 1, we must divide remainder by d */
    if (d > 1) {
        unsigned int rem_dummy = 0;
        BigInt *r2 = bigint_divmod_small(r, d, &rem_dummy);
        bigint_free(r);
        r = r2;
    }

    /* set signs */
    if (q->len == 1 && q->data[0] == 0) q->sign = 1; else q->sign = a->sign * b->sign;
    if (r->len == 1 && r->data[0] == 0) r->sign = 1; else r->sign = a->sign;

    if (q_out) *q_out = q; else bigint_free(q);
    if (r_out) *r_out = r; else bigint_free(r);

    free(u); free(v);
}

/* ------------------ GCD (MDC) via Euclides ------------------ */

BigInt *bigint_gcd(const BigInt *x, const BigInt *y) {
    if (!x || !y) return NULL;
    /* gcd should be on absolute values */
    BigInt *a = bigint_abs_copy(x);
    BigInt *b = bigint_abs_copy(y);
    if (!a || !b) { bigint_free(a); bigint_free(b); return NULL; }

    /* ensure a >= b */
    if (bigint_cmpabs(a, b) < 0) {
        BigInt *tmp = a; a = b; b = tmp;
    }

    while (!(b->len == 1 && b->data[0] == 0)) {
        BigInt *q = NULL;
        BigInt *r = NULL;
        bigint_divmod(a, b, &q, &r);
        bigint_free(q);
        bigint_free(a);
        a = b;
        b = r;
    }

    /* a is gcd, ensure positive sign */
    a->sign = 1;
    bigint_free(b);
    return a;
}

/* ------------------ IO ------------------ */
BigInt *read_bigint_stdin(void) {
    char buf[10005];
    if (!fgets(buf, sizeof(buf), stdin)) return NULL;
    size_t l = strlen(buf);
    if (l && buf[l-1] == '\n') buf[l-1] = '\0';
    return bigint_from_string(buf);
}

/* ------------------ Main (menu) ------------------ */

int main(void) {
    char line[64];

    while (1) {
        printf("=== CalcBigInt ===\n");
        printf("1) Soma\n");
        printf("2) Subtracao\n");
        printf("3) Multiplicacao\n");
        printf("4) Divisao inteira\n");
        printf("5) Modulo (resto)\n");
        printf("7) MDC (maximo divisor comum)\n");
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

        if (op < 1 || (op > 7 || op == 6)) {
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
        } else if (op == 7) {
            BigInt *g = bigint_gcd(a, b);
            if (!g) {
                printf("Erro no calculo do MDC.\n");
            } else {
                char *sg = bigint_to_string(g);
                if (sg) {
                    printf("MDC: %s\n", sg);
                    free(sg);
                }
                bigint_free(g);
            }
        }

        bigint_free(a);
        bigint_free(b);

        printf("\n");
    }

    return 0;
}
