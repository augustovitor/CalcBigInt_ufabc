#ifndef IO_H
#define IO_H

#include "bigint.h"

BigInt *read_bigint_stdin(void);
int write_bigint_to_file(const char *path, const BigInt *res);

#endif