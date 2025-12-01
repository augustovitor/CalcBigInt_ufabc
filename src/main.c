/* main.c - versão monolítica combinando bigint, operations, io e main.
 *
 * Originou-se dos arquivos enviados: bigint.c / bigint.h, operations.c / operations.h,
 * io.c / io.h e main.c.
 *
 * Observação: algoritmo de divisão é a versão simples por subtrações (correto, porém lento).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ------------------ bigint.h (conteúdo embutido) ------------------ */

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

/* ------------------ prototypes de operações (operations.h embutido) ------------------ */

BigInt *bigint_add(const BigInt *a, const BigInt *b);
BigInt *bigint_sub(const BigInt *a, const BigInt *b);
BigInt *bigint_mul(const BigInt *a, const BigInt *b);
void bigint_divmod(const BigInt *a, const BigInt *b, BigInt **q, BigInt **r);

/* ------------------ IO protótipos (io.h embutido) ------------------ */

BigInt *read_bigint_stdin(void);
int write_bigint_to_file(const char *path, const BigInt *res);

/* ------------------ bigint.c (implementação embutida) ------------------ */

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

/* ------------------ operations.c (implementação embutida) ------------------ */

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

/* ------------------ io.c (implementação embutida) ------------------ */

BigInt *read_bigint_stdin(void) {
    char buf[10005];
    if (!fgets(buf, sizeof(buf), stdin)) return NULL;
    size_t l = strlen(buf);
    if (l && buf[l-1] == '\n') buf[l-1] = '\0';
    return bigint_from_string(buf);
}

int write_bigint_to_file(const char *path, const BigInt *res) {
    char *s = bigint_to_string(res);
    FILE *f = fopen(path, "w");
    if (!f) { free(s); return -1; }
    fprintf(f, "%s\n", s);
    fclose(f);
    free(s);
    return 0;
}

/* ------------------ main.c (embutido) ------------------ */

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
