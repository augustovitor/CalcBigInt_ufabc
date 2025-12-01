# Makefile - MSYS2 MINGW64
CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11
TARGET = calcbigint.exe
SRC = src/main.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
