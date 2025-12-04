/* 
 *
 * Comportamento:
 * - sem argumentos: modo interativo (menu)
 * - com 1 argumento: trata o argumento como arquivo de entrada (ex: entrada.txt)
 *   - arquivo deve ter 3 linhas: operacao (ex: +, -, *, /, %, m), numero1, numero2
 *   - saída será escrita em "resultado.txt" (um único número, dependendo da operação)
 *
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

/* basic helpers */
int bigint_is_zero(const BigInt *a);

/* operações principais */
BigInt *bigint_add(const BigInt *a, const BigInt *b);
BigInt *bigint_sub(const BigInt *a, const BigInt *b);
BigInt *bigint_mul(const BigInt *a, const BigInt *b);
void bigint_divmod(const BigInt *a, const BigInt *b, BigInt **q, BigInt **r);

/* multiplicação por uint e shift (usados na divisão longa) */
BigInt *bigint_mul_uint(const BigInt *a, unsigned int m);
BigInt *bigint_shift_blocks(const BigInt *a, size_t shift);

/* gcd */
BigInt *bigint_gcd(const BigInt *a, const BigInt *b);

/* IO */
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

/* helpers */
int bigint_is_zero(const BigInt *a) {
    return (a->len == 1 && a->data[0] == 0);
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

/* multiplicar BigInt por uint (0 <= m < BASE) */
BigInt *bigint_mul_uint(const BigInt *a, unsigned int m) {
    if (!a) return NULL;
    if (m == 0) return bigint_from_uint(0);
    if (m == 1) return bigint_abs_copy(a);

    BigInt *res = bigint_new(a->len + 1, a->sign);
    if (!res) return NULL;

    unsigned long long carry = 0;
    for (size_t i = 0; i < a->len; ++i) {
        unsigned long long cur = (unsigned long long)a->data[i] * m + carry;
        res->data[i] = (unsigned int)(cur % BASE);
        carry = cur / BASE;
    }
    if (carry) res->data[a->len] = (unsigned int)carry;
    bigint_normalize(res);
    return res;
}

/* shift por blocos (multiplica por BASE^shift) */
BigInt *bigint_shift_blocks(const BigInt *a, size_t shift) {
    if (!a) return NULL;
    if (bigint_is_zero(a)) return bigint_from_uint(0);
    BigInt *res = bigint_new(a->len + shift, a->sign);
    if (!res) return NULL;
    /* data[0..shift-1] already zero due to calloc; copy */
    for (size_t i = 0; i < a->len; ++i) res->data[i + shift] = a->data[i];
    bigint_normalize(res);
    return res;
}

/* Divisão inteira e resto: a / b = q, a % b = r
   Implementação: divisão longa por blocos (base BASE).
   Para cada posição k (de n-m -> 0) fazemos busca binária do dígito qk (0..BASE-1):
   testamos (divisor << k) * mid <= dividend. Complexidade razoável e muito mais rápida
   que subtração repetida para números grandes.
*/
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

    /* cópia do dividendo (positivo para facilitar) */
    BigInt *dividend = bigint_abs_copy(a);
    BigInt *divisor  = bigint_abs_copy(b);

    size_t n = dividend->len;
    size_t m = divisor->len;

    size_t qlen = (n >= m) ? (n - m + 1) : 1;
    BigInt *quotient = bigint_new(qlen, a->sign * b->sign);
    if (!quotient) {
        bigint_free(dividend); bigint_free(divisor);
        return;
    }
    /* inicializa zeros já vêm do calloc */

    /* Para k de n-m down to 0 */
    for (ssize_t k = (ssize_t)(n - m); k >= 0; --k) {
        /* shifted = divisor * BASE^k */
        BigInt *shifted = bigint_shift_blocks(divisor, (size_t)k);
        if (!shifted) { bigint_free(dividend); bigint_free(divisor); bigint_free(quotient); return; }

        /* Se shifted > dividend, qk = 0 */
        if (bigint_cmpabs(shifted, dividend) > 0) {
            bigint_free(shifted);
            quotient->data[k] = 0;
            continue;
        }

        /* busca binária por qk em [0, BASE-1] */
        unsigned int lo = 0, hi = BASE - 1;
        unsigned int best = 0;

        while (lo <= hi) {
            unsigned int mid = lo + (hi - lo) / 2;
            BigInt *prod = bigint_mul_uint(shifted, mid);
            if (!prod) { bigint_free(shifted); bigint_free(dividend); bigint_free(divisor); bigint_free(quotient); return; }
            int c = bigint_cmpabs(prod, dividend);
            bigint_free(prod);
            if (c <= 0) {
                best = mid;
                lo = mid + 1;
            } else {
                if (mid == 0) break;
                hi = mid - 1;
            }
        }

        if (best > 0) {
            BigInt *bestprod = bigint_mul_uint(shifted, best);
            BigInt *tmp = bigint_sub(dividend, bestprod);
            bigint_free(dividend);
            bigint_free(bestprod);
            dividend = tmp;
        }
        quotient->data[k] = best;
        bigint_free(shifted);
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
}

/* ------------------ gcd (MDC) ------------------ */

/* Euclides*/
BigInt *bigint_gcd(const BigInt *a, const BigInt *b) {
    if (!a || !b) return NULL;

    BigInt *A = bigint_abs_copy(a);
    BigInt *B = bigint_abs_copy(b);
    if (!A || !B) { bigint_free(A); bigint_free(B); return NULL; }

    
    while (!bigint_is_zero(B)) {
        BigInt *R = NULL;
        bigint_divmod(A, B, NULL, &R);
        bigint_free(A);
        A = B;
        B = R;
    }

  
    A->sign = 1;
    if (B) bigint_free(B);
    return A;
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

static void print_menu(void) {
    printf("=== CalcBigInt ===\n");
    printf("1) Soma (+)\n");
    printf("2) Subtracao (-)\n");
    printf("3) Multiplicacao (*)\n");
    printf("4) Divisao inteira (/)\n");
    printf("5) Modulo (%%)\n");
    printf("6) MDC\n");
    printf("7) Sair\n");
    printf("Escolha: ");
}

/* lê arquivo de entrada com 3 linhas: op, a, b
   retorna op char em *op_out e BigInt* em a_out, b_out (callees deve liberar)
   retorna 0 em sucesso, -1 em erro.
*/
static int read_from_file_input(const char *fname, char *op_out, BigInt **a_out, BigInt **b_out) {
    FILE *f = fopen(fname, "r");
    if (!f) return -1;
    char line[10010];

    /* linha 1: operação */
    if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
    size_t l = strlen(line); if (l && line[l-1]=='\n') line[l-1]='\0';
    
    char *p = line;
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) { fclose(f); return -1; }
    char op = *p;

    /* linha 2: a */
    if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
    l = strlen(line); if (l && line[l-1]=='\n') line[l-1]='\0';
    BigInt *a = bigint_from_string(line);
    if (!a) { fclose(f); return -1; }

    /* linha 3: b */
    if (!fgets(line, sizeof(line), f)) { bigint_free(a); fclose(f); return -1; }
    l = strlen(line); if (l && line[l-1]=='\n') line[l-1]='\0';
    BigInt *b = bigint_from_string(line);
    if (!b) { bigint_free(a); fclose(f); return -1; }

    fclose(f);
    *op_out = op;
    *a_out = a;
    *b_out = b;
    return 0;
}

int main(int argc, char **argv) {
    char line[64];

    /* comportamento: se argc == 2 -> modo arquivo. escreve "resultado.txt" */
    if (argc == 2) {
        const char *inputfile = argv[1];
        const char *outputfile = "resultado.txt";

        char op;
        BigInt *a = NULL, *b = NULL;
        if (read_from_file_input(inputfile, &op, &a, &b) != 0) {
            fprintf(stderr, "Erro lendo arquivo de entrada (%s)\n", inputfile);
            return 1;
        }

        BigInt *res_q = NULL, *res_r = NULL, *res = NULL;
        if (op == '+') {
            res = bigint_add(a, b);
        } else if (op == '-') {
            res = bigint_sub(a, b);
        } else if (op == '*') {
            res = bigint_mul(a, b);
        } else if (op == '/') {
            bigint_divmod(a, b, &res_q, &res_r);
            /* para '/' escrevemos apenas o quociente */
            if (res_q) {
                write_bigint_to_file(outputfile, res_q);
            } else {
                fprintf(stderr, "Erro na divisao (divisor zero?).\n");
                bigint_free(a); bigint_free(b);
                if (res_q) bigint_free(res_q);
                if (res_r) bigint_free(res_r);
                return 1;
            }
            bigint_free(a); bigint_free(b);
            if (res_q) bigint_free(res_q);
            if (res_r) bigint_free(res_r);
            printf("Resultado escrito em %s\n", outputfile);
            return 0;
        } else if (op == '%') {
            bigint_divmod(a, b, &res_q, &res_r);
            /* para '%' escrevemos apenas o resto */
            if (res_r) {
                write_bigint_to_file(outputfile, res_r);
            } else {
                fprintf(stderr, "Erro na operacao modulo (divisor zero?).\n");
                bigint_free(a); bigint_free(b);
                if (res_q) bigint_free(res_q);
                if (res_r) bigint_free(res_r);
                return 1;
            }
            bigint_free(a); bigint_free(b);
            if (res_q) bigint_free(res_q);
            if (res_r) bigint_free(res_r);
            printf("Resultado escrito em %s\n", outputfile);
            return 0;
        } else if (op == 'm' || op == 'M') {
            BigInt *g = bigint_gcd(a, b);
            if (!g) {
                fprintf(stderr, "Erro calculando MDC.\n");
                bigint_free(a); bigint_free(b);
                return 1;
            }
            if (write_bigint_to_file(outputfile, g) != 0) {
                fprintf(stderr, "Erro escrevendo resultado em %s\n", outputfile);
                bigint_free(g); bigint_free(a); bigint_free(b);
                return 1;
            }
            printf("Resultado escrito em %s\n", outputfile);
            bigint_free(g); bigint_free(a); bigint_free(b);
            return 0;
        } else {
            fprintf(stderr, "Operacao desconhecida: %c\n", op);
            bigint_free(a); bigint_free(b);
            return 1;
        }

        if (!res) {
            fprintf(stderr, "Erro na operacao.\n");
            bigint_free(a); bigint_free(b);
            return 1;
        }
        /* para + - * escrevemos resultado no arquivo */
        if (write_bigint_to_file(outputfile, res) != 0) {
            fprintf(stderr, "Erro escrevendo resultado em %s\n", outputfile);
            bigint_free(res); bigint_free(a); bigint_free(b);
            return 1;
        }
        printf("Resultado escrito em %s\n", outputfile);
        bigint_free(res); bigint_free(a); bigint_free(b);
        return 0;
    }

    /* modo interativo */
    while (1) {
        print_menu();

        if (!fgets(line, sizeof(line), stdin)) break;
        int opnum = 0;
        if (sscanf(line, "%d", &opnum) != 1) {
            printf("Opcao invalida.\n\n");
            continue;
        }

        if (opnum == 7) {
            printf("Saindo...\n");
            break;
        }

        if (opnum < 1 || opnum > 7) {
            printf("Opcao invalida.\n\n");
            continue;
        }

        printf("Digite o primeiro numero:\n");
        BigInt *a = read_bigint_stdin();
        if (!a) { printf("Erro na leitura.\n"); continue; }

        printf("Digite o segundo numero:\n");
        BigInt *b = read_bigint_stdin();
        if (!b) { printf("Erro na leitura.\n"); bigint_free(a); continue; }

        if (opnum == 1 || opnum == 2 || opnum == 3) {
            BigInt *res = NULL;
            if (opnum == 1) res = bigint_add(a, b);
            else if (opnum == 2) res = bigint_sub(a, b);
            else if (opnum == 3) res = bigint_mul(a, b);

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
        } else if (opnum == 4) {
            /* Divisao inteira: mostrar apenas quociente */
            BigInt *q = NULL;
            BigInt *r = NULL;
            bigint_divmod(a, b, &q, &r);
            if (!q) {
                printf("Erro na divisao (divisor zero?).\n");
            } else {
                char *sq = bigint_to_string(q);
                if (sq) { printf("%s\n", sq); free(sq); }
            }
            if (q) bigint_free(q);
            if (r) bigint_free(r);
        } else if (opnum == 5) {
            /* Modulo: mostrar apenas resto */
            BigInt *q = NULL;
            BigInt *r = NULL;
            bigint_divmod(a, b, &q, &r);
            if (!r) {
                printf("Erro na operacao modulo (divisor zero?).\n");
            } else {
                char *sr = bigint_to_string(r);
                if (sr) { printf("%s\n", sr); free(sr); }
            }
            if (q) bigint_free(q);
            if (r) bigint_free(r);
        } else if (opnum == 6) {
            /* MDC */
            BigInt *g = bigint_gcd(a, b);
            if (!g) {
                printf("Erro calculando MDC.\n");
            } else {
                char *sg = bigint_to_string(g);
                if (sg) { printf("%s\n", sg); free(sg); }
                bigint_free(g);
            }
        }

        bigint_free(a);
        bigint_free(b);

        printf("\n");
    }

    return 0;
}
