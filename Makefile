# Makefile - projeto CalcBigInt
# Suporta estrutura: (Makefile aqui) + src/main.c (arquivo único monolítico)

CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11
SRCDIR = src
SRC = $(SRCDIR)/main.c
TARGET = calcbigint.exe

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
