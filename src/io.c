#include "io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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