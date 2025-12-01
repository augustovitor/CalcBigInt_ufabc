/* main.c - monolítico com bigint + operações + IO
 *
 * Novidades:
 *  - algoritmo de divisão mais eficiente (long division por base)
 *  - operação MDC (gcd) pelo algoritmo de Euclides
 *  - leitura opcional de arquivo .txt com 3 linhas: operação, num1, num2
 *
 * Compilar: make (Makefile fornecido)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ------------------ configuração de biginteger ------------------ */

typedef struct {
    int sign;              /* +1 ou -1 */
    size_t len;            /* quantidade de blocos usados em data */
    unsigned int *data;    /* blocos em base BASE, little-endian (data[0] = menos significativo) */
} BigInt;

#define BASE 1000000000u   /* 1e9 */
#define BASE_DIGITS 9

/* protótipos */
BigInt *bigint_from_string(const char *s);
char *bigint_to_string(const BigInt *a);
void bigint_free(BigInt *a);
int bigint_cmpabs(const BigInt *a, const BigInt *b);
void bigint_normalize(BigInt *a);

BigInt *bigint_add(const BigInt *a, const BigInt *b);
BigInt *bigint_sub(const BigInt *a, const BigInt *b);
BigInt *bigint_mul(const BigInt *a, const BigInt *b);
void bigint_divmod(const BigInt *a, const BigInt *b, BigInt **q, BigInt **r);

BigInt *bigint_gcd(const BigInt *a, const BigInt *b);

/* utils internos */
static BigInt *bigint_new(size_t len, int sign);
static BigInt *bigint_from_uint(unsigned int v);
static BigInt *bigint_abs_copy(const BigInt *x);
static BigInt *bigint_mul_uint(const BigInt *a, unsigned int m);
static void bigint_mul_inplace_uint(BigInt *a, unsigned int m);
static void bigint_add_inplace_uint(BigInt *a, unsigned int v);

/* ------------------ implementação ------------------ */

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

    /* valida caracteres */
    for (size_t i = 0; i < slen; ++i) if (!isdigit((unsigned char)s[i])) return NULL;

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

static BigInt *bigint_from_uint(unsigned int v) {
    BigInt *r = bigint_new(1, 1);
    if (!r) return NULL;
    r->data[0] = v;
    bigint_normalize(r);
    return r;
}

static BigInt *bigint_abs_copy(const BigInt *x) {
    BigInt *r = bigint_new(x->len, 1);
    if (!r) return NULL;
    memcpy(r->data, x->data, x->len * sizeof(unsigned int));
    bigint_normalize(r);
    return r;
}

/* multiplicacao por inteiro pequeno -> retorna novo BigInt */
static BigInt *bigint_mul_uint(const BigInt *a, unsigned int m) {
    if (!a) return NULL;
    if (m == 0) return bigint_from_uint(0);
    if (m == 1) return bigint_abs_copy(a);

    BigInt *res = bigint_new(a->len + 1, 1);
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

/* multiplica inplace por inteiro pequeno */
static void bigint_mul_inplace_uint(BigInt *a, unsigned int m) {
    if (!a) return;
    if (m == 0) {
        a->len = 1;
        a->data[0] = 0;
        a->sign = 1;
        return;
    }
    if (m == 1) return;
    unsigned long long carry = 0;
    for (size_t i = 0; i < a->len; ++i) {
        unsigned long long cur = (unsigned long long)a->data[i] * m + carry;
        a->data[i] = (unsigned int)(cur % BASE);
        carry = cur / BASE;
    }
    if (carry) {
        size_t old = a->len;
        a->data = realloc(a->data, (old + 1) * sizeof(unsigned int));
        a->data[old] = (unsigned int)carry;
        a->len = old + 1;
    }
    bigint_normalize(a);
}

/* soma inplace por inteiro pequeno (v < BASE) */
static void bigint_add_inplace_uint(BigInt *a, unsigned int v) {
    if (!a) return;
    unsigned long long carry = v;
    size_t i = 0;
    while (carry && i < a->len) {
        unsigned long long cur = (unsigned long long)a->data[i] + carry;
        a->data[i] = (unsigned int)(cur % BASE);
        carry = cur / BASE;
        ++i;
    }
    if (carry) {
        a->data = realloc(a->data, (a->len + 1) * sizeof(unsigned int));
        a->data[a->len] = (unsigned int)carry;
        a->len++;
    }
}

/* soma absoluta */
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

/* subtrai |a| - |b| assumindo |a| >= |b| */
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

/* subtracao geral: a - b */
BigInt *bigint_sub(const BigInt *a, const BigInt *b) {
    if (!a || !b) return NULL;
    BigInt tmp = *b;
    tmp.sign = -b->sign;
    return bigint_add(a, &tmp);
}

/* multiplicacao: a * b */
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

/* ------------------ divisão mais eficiente (schoolbook by base) ------------------
   Algoritmo:
   - percorre os blocos de 'a' do mais significativo ao menos (i = a.len-1 .. 0)
   - acumula remainder := remainder * BASE + a.data[i]
   - encontra qdigit em [0, BASE-1] tal que b*qdigit <= remainder < b*(qdigit+1)
     (usa busca binária; b*mid é obtido por bigint_mul_uint)
   - coloca qdigit em quotient.data[i], remainder := remainder - b*qdigit
   Complexidade: O(n * (log BASE) * mul_small_cost)
   ----------------------------------------------------------------------------- */
void bigint_divmod(const BigInt *a, const BigInt *b, BigInt **q, BigInt **r) {
    if (q) *q = NULL;
    if (r) *r = NULL;
    if (!a || !b) return;

    /* divisor zero -> erro (retorna nada) */
    if (b->len == 1 && b->data[0] == 0) {
        return;
    }

    /* if |a| < |b| -> q = 0, r = a */
    if (bigint_cmpabs(a, b) < 0) {
        if (q) *q = bigint_from_uint(0);
        if (r) {
            *r = bigint_abs_copy(a);
            if (*r) (*r)->sign = a->sign;
        }
        return;
    }

    /* initialize quotient with length = a->len */
    BigInt *quot = bigint_new(a->len, a->sign * b->sign);
    if (!quot) return;

    /* remainder initially 0 */
    BigInt *rem = bigint_from_uint(0);
    if (!rem) { bigint_free(quot); return; }

    /* process digits from most significant to least */
    for (ssize_t idx = (ssize_t)a->len - 1; idx >= 0; --idx) {
        /* rem = rem * BASE + a->data[idx] */
        bigint_mul_inplace_uint(rem, BASE);
        bigint_add_inplace_uint(rem, a->data[idx]);

        /* binary search qdigit in [0, BASE-1] */
        unsigned int lo = 0, hi = BASE - 1, best = 0;
        while (lo <= hi) {
            unsigned int mid = lo + ((hi - lo) >> 1);
            BigInt *prod = bigint_mul_uint(b, mid);
            if (!prod) { /* allocation failure: cleanup and abort */
                bigint_free(rem);
                bigint_free(quot);
                return;
            }
            int cmp = bigint_cmpabs(prod, rem);
            bigint_free(prod);
            if (cmp <= 0) {
                best = mid;
                lo = mid + 1;
            } else {
                if (mid == 0) break;
                hi = mid - 1;
            }
        }

        quot->data[idx] = best;

        if (best) {
            BigInt *prod = bigint_mul_uint(b, best);
            BigInt *newrem = bigint_sub_abs(rem, prod);
            bigint_free(prod);
            bigint_free(rem);
            rem = newrem;
        }
    }

    bigint_normalize(quot);
    bigint_normalize(rem);

    if (quot->len == 1 && quot->data[0] == 0) quot->sign = 1;
    if (rem->len == 1 && rem->data[0] == 0) rem->sign = 1;
    else rem->sign = a->sign;

    if (q) *q = quot; else bigint_free(quot);
    if (r) *r = rem; else bigint_free(rem);
}

/* gcd via algoritmo de Euclides: gcd(a,b) = gcd(b, a % b) */
BigInt *bigint_gcd(const BigInt *a, const BigInt *b) {
    if (!a || !b) return NULL;

    /* copia valores absolutos */
    BigInt *x = bigint_abs_copy(a);
    BigInt *y = bigint_abs_copy(b);
    if (!x || !y) { bigint_free(x); bigint_free(y); return NULL; }

    /* garantir ordem: x >= y */
    if (bigint_cmpabs(x, y) < 0) {
        BigInt *tmp = x; x = y; y = tmp;
    }

    while (!(y->len == 1 && y->data[0] == 0)) {
        BigInt *q = NULL, *r = NULL;
        bigint_divmod(x, y, &q, &r);
        /* descartamos q */
        if (q) bigint_free(q);
        bigint_free(x);
        x = y;
        y = r;
    }

    /* x contém gcd (positivo) */
    if (y) bigint_free(y);
    x->sign = 1;
    return x;
}

/* ------------------ IO / interface ------------------ */

BigInt *read_bigint_stdin(void) {
    char buf[10005];
    if (!fgets(buf, sizeof(buf), stdin)) return NULL;
    size_t l = strlen(buf);
    if (l && buf[l-1] == '\n') buf[l-1] = '\0';
    return bigint_from_string(buf);
}

int read_file_three_lines(const char *path, int *out_op, BigInt **out_a, BigInt **out_b) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    char line[10050];

    /* linha 1: operação */
    if (!fgets(line, sizeof(line), f)) { fclose(f); return -2; }
    size_t l = strlen(line); if (l && line[l-1] == '\n') line[l-1] = '\0';
    char opstr[128];
    strncpy(opstr, line, sizeof(opstr)); opstr[sizeof(opstr)-1] = '\0';

    /* linha 2: numero a */
    if (!fgets(line, sizeof(line), f)) { fclose(f); return -3; }
    l = strlen(line); if (l && line[l-1] == '\n') line[l-1] = '\0';
    char a_str[10050]; strncpy(a_str, line, sizeof(a_str)); a_str[sizeof(a_str)-1] = '\0';

    /* linha 3: numero b */
    if (!fgets(line, sizeof(line), f)) { fclose(f); return -4; }
    l = strlen(line); if (l && line[l-1] == '\n') line[l-1] = '\0';
    char b_str[10050]; strncpy(b_str, line, sizeof(b_str)); b_str[sizeof(b_str)-1] = '\0';

    fclose(f);

    /* interpreta op: pode ser número 1..7 ou texto */
    int op = 0;
    /* trim */
    char *p = opstr; while (*p && isspace((unsigned char)*p)) p++;
    if (isdigit((unsigned char)*p)) {
        op = atoi(p);
    } else {
        /* compara textual (case-insensitive) */
        for (char *q = p; *q; ++q) *q = (char)tolower((unsigned char)*q);
        if (strstr(p, "soma") || strstr(p, "add") || strcmp(p, "1") == 0) op = 1;
        else if (strstr(p, "sub") || strstr(p, "subtr") || strcmp(p, "2") == 0) op = 2;
        else if (strstr(p, "mul") || strstr(p, "mult") || strcmp(p, "3") == 0) op = 3;
        else if (strstr(p, "div") || strstr(p, "divisao") || strcmp(p, "4") == 0) op = 4;
        else if (strstr(p, "mod") || strstr(p, "resto") || strcmp(p, "5") == 0) op = 5;
        else if (strstr(p, "mdc") || strstr(p, "gcd") || strcmp(p, "6") == 0) op = 6;
        else if (strstr(p, "sair") || strcmp(p, "7") == 0) op = 7;
        else return -5; /* operação desconhecida */
    }

    BigInt *A = bigint_from_string(a_str);
    BigInt *B = bigint_from_string(b_str);
    if (!A || !B) { bigint_free(A); bigint_free(B); return -6; }

    *out_op = op;
    *out_a = A;
    *out_b = B;
    return 0;
}

/* escreve bigint em arquivo */
int write_bigint_to_file(const char *path, const BigInt *res) {
    char *s = bigint_to_string(res);
    FILE *f = fopen(path, "w");
    if (!f) { free(s); return -1; }
    fprintf(f, "%s\n", s);
    fclose(f);
    free(s);
    return 0;
}

/* ------------------ interface principal ------------------ */

static void print_menu(void) {
    printf("=== CalcBigInt ===\n");
    printf("1) Soma\n");
    printf("2) Subtracao\n");
    printf("3) Multiplicacao\n");
    printf("4) Divisao inteira\n");
    printf("5) Modulo (resto)\n");
    printf("6) MDC (Maior Divisor Comum)\n");
    printf("7) Sair\n");
    printf("Escolha: ");
}

int main(int argc, char **argv) {
    /* modo arquivo se for passado 1 argumento (arquivo) */
    if (argc == 2) {
        int op = 0;
        BigInt *A = NULL, *B = NULL;
        int rc = read_file_three_lines(argv[1], &op, &A, &B);
        if (rc != 0) {
            fprintf(stderr, "Erro lendo arquivo (%d)\n", rc);
            return 1;
        }

        if (op == 7) { /* sair */
            bigint_free(A); bigint_free(B);
            return 0;
        }

        if (op < 1 || op > 6) {
            fprintf(stderr, "Operacao invalida no arquivo: %d\n", op);
            bigint_free(A); bigint_free(B);
            return 1;
        }

        if (op == 1 || op == 2 || op == 3) {
            BigInt *res = NULL;
            if (op == 1) res = bigint_add(A, B);
            else if (op == 2) res = bigint_sub(A, B);
            else if (op == 3) res = bigint_mul(A, B);

            if (!res) { fprintf(stderr, "Erro na operacao\n"); }
            else {
                char *s = bigint_to_string(res);
                if (s) {
                    printf("%s\n", s);
                    free(s);
                }
                bigint_free(res);
            }
        } else if (op == 4 || op == 5) {
            BigInt *q = NULL, *r = NULL;
            bigint_divmod(A, B, &q, &r);
            if (!q && !r) {
                fprintf(stderr, "Erro na divisao (talvez divisor zero)\n");
            } else {
                if (op == 4 && q) {
                    char *sq = bigint_to_string(q);
                    if (sq) { printf("%s\n", sq); free(sq); }
                }
                if (op == 5 && r) {
                    char *sr = bigint_to_string(r);
                    if (sr) { printf("%s\n", sr); free(sr); }
                }
            }
            if (q) bigint_free(q); if (r) bigint_free(r);
        } else if (op == 6) {
            BigInt *g = bigint_gcd(A, B);
            if (!g) { fprintf(stderr, "Erro calculando MDC\n"); }
            else {
                char *sg = bigint_to_string(g);
                if (sg) { printf("%s\n", sg); free(sg); }
                bigint_free(g);
            }
        }

        bigint_free(A); bigint_free(B);
        return 0;
    }

    /* modo interativo (menu) */
    char line[64];
    while (1) {
        print_menu();
        if (!fgets(line, sizeof(line), stdin)) break;
        int op = 0;
        if (sscanf(line, "%d", &op) != 1) {
            printf("Opcao invalida.\n\n");
            continue;
        }
        if (op == 7) { printf("Saindo...\n"); break; }
        if (op < 1 || op > 6) { printf("Opcao invalida.\n\n"); continue; }

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
                    if (sq) { printf("Quociente: %s\n", sq); free(sq); }
                }
                if (op == 5 && r) {
                    char *sr = bigint_to_string(r);
                    if (sr) { printf("Resto: %s\n", sr); free(sr); }
                }
            }

            if (q) bigint_free(q);
            if (r) bigint_free(r);
        } else if (op == 6) {
            BigInt *g = bigint_gcd(a, b);
            if (!g) {
                printf("Erro no calculo do MDC.\n");
            } else {
                char *sg = bigint_to_string(g);
                if (sg) { printf("MDC: %s\n", sg); free(sg); }
                bigint_free(g);
            }
        }

        bigint_free(a);
        bigint_free(b);
        printf("\n");
    }

    return 0;
}
