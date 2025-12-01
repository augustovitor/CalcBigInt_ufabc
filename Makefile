CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11
TARGET = calc
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
