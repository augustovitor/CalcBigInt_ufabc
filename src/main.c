#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ============================================================
   BIGINT STRUCT
   ============================================================ */
typedef struct {
    int sign;
    size_t len;
    unsigned int *data;
} BigInt;

#define BASE 1000000000u
#define BASE_DIGITS 9

/* ============================================================
   PROTÓTIPOS
   ============================================================ */
BigInt *bigint_from_string(const char *s);
char *bigint_to_string(const BigInt *a);
void bigint_free(BigInt *a);
void bigint_normalize(BigInt *a);
int bigint_cmpabs(const BigInt *a, const BigInt *b);

BigInt *bigint_add(const BigInt *a, const BigInt *b);
BigInt *bigint_sub(const BigInt *a, const BigInt *b);
BigInt *bigint_mul(const BigInt *a, const BigInt *b);
void bigint_divmod(const BigInt *a, const BigInt *b, BigInt **q, BigInt **r);
BigInt *bigint_gcd(BigInt *a, BigInt *b);

BigInt *read_bigint_stdin(void);
BigInt *read_bigint_from_file_line(FILE *f);
int write_bigint_to_file(const char *path, const BigInt *res);

/* ============================================================
   BIGINT IMPLEMENTAÇÃO
   ============================================================ */
BigInt *bigint_from_string(const char *s) {
    while (*s == ' ' || *s == '\t' || *s == '\n') s++;

    int sign = 1;
    if (*s == '-') sign = -1, s++;
    else if (*s == '+') s++;

    while (*s == '0') s++;

    if (*s == '\0') {
        BigInt *z = malloc(sizeof(BigInt));
        z->sign = 1;
        z->len = 1;
        z->data = calloc(1, sizeof(unsigned int));
        z->data[0] = 0;
        return z;
    }

    size_t slen = strlen(s);
    size_t n = (slen + BASE_DIGITS - 1) / BASE_DIGITS;

    BigInt *res = malloc(sizeof(BigInt));
    res->sign = sign;
    res->len = n;
    res->data = calloc(n, sizeof(unsigned int));

    size_t idx = 0;
    for (ssize_t i = slen; i > 0; i -= BASE_DIGITS) {
        ssize_t start = i - BASE_DIGITS;
        if (start < 0) start = 0;

        char buf[BASE_DIGITS + 1];
        memcpy(buf, s + start, i - start);
        buf[i - start] = '\0';

        res->data[idx++] = atoi(buf);
    }

    return res;
}

char *bigint_to_string(const BigInt *a) {
    if (a->len == 1 && a->data[0] == 0) {
        char *z = malloc(2);
        strcpy(z, "0");
        return z;
    }

    char *out = malloc(a->len * BASE_DIGITS + 3);
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
    while (a->len > 1 && a->data[a->len - 1] == 0)
        a->len--;

    if (a->len == 1 && a->data[0] == 0)
        a->sign = 1;
}

int bigint_cmpabs(const BigInt *a, const BigInt *b) {
    if (a->len != b->len)
        return (a->len > b->len) ? 1 : -1;

    for (size_t i = a->len; i-- > 0;) {
        if (a->data[i] != b->data[i])
            return (a->data[i] > b->data[i]) ? 1 : -1;
    }
    return 0;
}

/* ============================================================
   OPERAÇÕES BÁSICAS
   ============================================================ */

BigInt *new_bigint(size_t len, int sign) {
    BigInt *r = malloc(sizeof(BigInt));
    r->data = calloc(len, sizeof(unsigned int));
    r->len = len;
    r->sign = sign;
    return r;
}

BigInt *bigint_add(const BigInt *a, const BigInt *b) {
    if (a->sign == b->sign) {
        size_t n = (a->len > b->len ? a->len : b->len);
        BigInt *r = new_bigint(n + 1, a->sign);

        unsigned long long carry = 0;
        for (size_t i = 0; i < n; i++) {
            unsigned long long av = (i < a->len ? a->data[i] : 0);
            unsigned long long bv = (i < b->len ? b->data[i] : 0);
            unsigned long long s = av + bv + carry;
            r->data[i] = s % BASE;
            carry = s / BASE;
        }
        if (carry) r->data[n] = carry;
        bigint_normalize(r);
        return r;
    }

    BigInt tmp = *b;
    tmp.sign = -tmp.sign;
    return bigint_sub(a, &tmp);
}

BigInt *bigint_sub(const BigInt *a, const BigInt *b) {
    if (a->sign != b->sign) {
        BigInt tmp = *b;
        tmp.sign = -tmp.sign;
        return bigint_add(a, &tmp);
    }

    int cmp = bigint_cmpabs(a, b);
    if (cmp == 0) {
        BigInt *z = new_bigint(1, 1);
        z->data[0] = 0;
        return z;
    }

    const BigInt *ma = (cmp > 0 ? a : b);
    const BigInt *mi = (cmp > 0 ? b : a);
    int sign = (cmp > 0 ? a->sign : -a->sign);

    BigInt *r = new_bigint(ma->len, sign);

    long long carry = 0;
    for (size_t i = 0; i < ma->len; i++) {
        long long av = ma->data[i];
        long long bv = (i < mi->len ? mi->data[i] : 0);

        long long s = av - bv + carry;
        if (s < 0) {
            s += BASE;
            carry = -1;
        } else carry = 0;

        r->data[i] = s;
    }

    bigint_normalize(r);
    return r;
}

BigInt *bigint_mul(const BigInt *a, const BigInt *b) {
    BigInt *r = new_bigint(a->len + b->len, a->sign * b->sign);

    for (size_t i = 0; i < a->len; i++) {
        unsigned long long carry = 0;

        for (size_t j = 0; j < b->len; j++) {
            unsigned long long cur =
                r->data[i + j] +
                (unsigned long long)a->data[i] * b->data[j] +
                carry;

            r->data[i + j] = cur % BASE;
            carry = cur / BASE;
        }

        r->data[i + b->len] += carry;
    }

    bigint_normalize(r);
    return r;
}

/* ============================================================
   DIVISÃO RÁPIDA — DIVISÃO LONGA EM BASE 1e9
   ============================================================ */

void bigint_divmod(const BigInt *a, const BigInt *b, BigInt **q, BigInt **r) {
    if (b->len == 1 && b->data[0] == 0) return;

    BigInt *quot = new_bigint(a->len, a->sign * b->sign);
    BigInt *rem = new_bigint(a->len, 1);

    memcpy(rem->data, a->data, a->len * sizeof(unsigned int));
    rem->len = a->len;

    BigInt *div = new_bigint(b->len, 1);
    memcpy(div->data, b->data, b->len * sizeof(unsigned int));
    div->len = b->len;

    bigint_normalize(rem);
    bigint_normalize(div);

    if (bigint_cmpabs(rem, div) < 0) {
        quotient:
        quot->len = 1;
        quot->data[0] = 0;

        if (q) *q = quot;
        else bigint_free(quot);

        if (r) *r = rem;
        else bigint_free(rem);

        bigint_free(div);
        return;
    }

    size_t n = rem->len;
    size_t m = div->len;

    unsigned long long d = BASE / (div->data[m - 1] + 1);

    if (d > 1) {
        BigInt *D = bigint_mul(rem, new_bigint(1, 1));
        BigInt *E = bigint_mul(div, new_bigint(1, 1));
    }

    for (size_t k = n - m; k < (size_t)-1; k--) {
        unsigned long long r2 = (unsigned long long)rem->data[k + m] * BASE +
                                rem->data[k + m - 1];
        unsigned long long qhat = r2 / div->data[m - 1];
        if (qhat >= BASE) qhat = BASE - 1;

        unsigned long long carry = 0, borrow = 0;
        for (size_t j = 0; j < m; j++) {
            unsigned long long prod = qhat * div->data[j];
            unsigned long long sub = rem->data[k + j] - (prod % BASE) - borrow;
            borrow = prod / BASE;

            if ((long long)sub < 0) {
                sub += BASE;
                borrow++;
            }

            rem->data[k + j] = sub;
        }

        unsigned long long sub2 = rem->data[k + m] - borrow;
        if ((long long)sub2 < 0) {
            sub2 += BASE;

            qhat--;
            carry = 0;

            for (size_t j = 0; j < m; j++) {
                unsigned long long sum =
                    rem->data[k + j] + div->data[j] + carry;

                rem->data[k + j] = sum % BASE;
                carry = sum / BASE;
            }
            rem->data[k + m] += carry;
        }

        quot->data[k] = qhat;
    }

    bigint_normalize(quot);
    bigint_normalize(rem);

    if (q) *q = quot;
    else bigint_free(quot);

    if (r) *r = rem;
    else bigint_free(rem);

    bigint_free(div);
}

/* ============================================================
   MDC — EUCLIDES (com repetidas divisões rápidas)
   ============================================================ */

BigInt *bigint_gcd(BigInt *a, BigInt *b) {
    BigInt *A = new_bigint(a->len, 1);
    memcpy(A->data, a->data, a->len * sizeof(unsigned int));
    A->len = a->len;

    BigInt *B = new_bigint(b->len, 1);
    memcpy(B->data, b->data, b->len * sizeof(unsigned int));
    B->len = b->len;

    while (!(B->len == 1 && B->data[0] == 0)) {
        BigInt *q, *r;
        bigint_divmod(A, B, &q, &r);
        bigint_free(q);
        bigint_free(A);
        A = B;
        B = r;
    }

    bigint_free(B);
    return A;
}

/* ============================================================
   IO
   ============================================================ */

BigInt *read_bigint_stdin(void) {
    char buf[10000];
    fgets(buf, sizeof(buf), stdin);
    return bigint_from_string(buf);
}

BigInt *read_bigint_from_file_line(FILE *f) {
    char buf[10000];
    if (!fgets(buf, sizeof(buf), f)) return NULL;

    size_t l = strlen(buf);
    if (l > 0 && buf[l - 1] == '\n') buf[l - 1] = '\0';

    return bigint_from_string(buf);
}

int write_bigint_to_file(const char *path, const BigInt *res) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;

    char *s = bigint_to_string(res);
    fprintf(f, "%s\n", s);
    free(s);
    fclose(f);
    return 0;
}

/* ============================================================
   MAIN
   ============================================================ */

int main(int argc, char *argv[]) {

    /* ======================================================
       MODO ARQUIVO AUTOMÁTICO
       ====================================================== */

    if (argc == 1) {
        printf("Modo Interativo:\n");
        printf("(Use entrada.txt e resultado.txt para o modo arquivo)\n\n");
    }

    if (argc == 2) {
        printf("Uso: ./calc entrada.txt resultado.txt\n");
        return 1;
    }

    if (argc == 3) {
        char *entrada = argv[1];
        char *saida = argv[2];

        FILE *f = fopen(entrada, "r");
        if (!f) {
            printf("Erro abrindo arquivo %s\n", entrada);
            return 1;
        }

        char op_str[50];
        fgets(op_str, sizeof(op_str), f);
        int op = atoi(op_str);

        BigInt *a = read_bigint_from_file_line(f);
        BigInt *b = read_bigint_from_file_line(f);
        fclose(f);

        BigInt *r = NULL, *q = NULL;

        switch (op) {
            case 1: r = bigint_add(a, b); break;
            case 2: r = bigint_sub(a, b); break;
            case 3: r = bigint_mul(a, b); break;
            case 4: bigint_divmod(a, b, &q, &r); break;
            case 5: bigint_divmod(a, b, &q, &r); break;
            case 6: r = bigint_gcd(a, b); break;
            default: printf("Operação inválida.\n"); return 1;
        }

        write_bigint_to_file(saida, (op == 4 ? q : r));

        bigint_free(a);
        bigint_free(b);
        if (q) bigint_free(q);
        if (r) bigint_free(r);

        return 0;
    }

    /* ======================================================
       MODO INTERATIVO (MENU NORMAL)
       ====================================================== */

    while (1) {
        printf("=== CalcBigInt ===\n");
        printf("1) Soma\n");
        printf("2) Subtração\n");
        printf("3) Multiplicação\n");
        printf("4) Divisão\n");
        printf("5) Resto\n");
        printf("6) MDC\n");
        printf("7) Sair\n");
        printf("Escolha: ");

        char buf[20];
        fgets(buf, sizeof(buf), stdin);
        int op = atoi(buf);

        if (op == 7) break;
        if (op < 1 || op > 6) continue;

        printf("Digite o primeiro número:\n");
        BigInt *a = read_bigint_stdin();

        printf("Digite o segundo número:\n");
        BigInt *b = read_bigint_stdin();

        BigInt *r = NULL, *q = NULL;

        switch (op) {
            case 1: r = bigint_add(a, b); break;
            case 2: r = bigint_sub(a, b); break;
            case 3: r = bigint_mul(a, b); break;
            case 4: bigint_divmod(a, b, &q, &r); break;
            case 5: bigint_divmod(a, b, &q, &r); break;
            case 6: r = bigint_gcd(a, b); break;
        }

        char *s = bigint_to_string(op == 4 ? q : r);
        printf("Resultado: %s\n\n", s);
        free(s);

        bigint_free(a);
        bigint_free(b);
        if (r) bigint_free(r);
        if (q) bigint_free(q);
    }

    return 0;
}
