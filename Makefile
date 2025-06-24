CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -g
TARGET=objdumplite

all: $(TARGET)

$(TARGET): objdumplite.c objdumplite.h
	$(CC) $(CFLAGS) -o $(TARGET) objdumplite.c

clean:
	rm -f $(TARGET)

.PHONY: all clean 