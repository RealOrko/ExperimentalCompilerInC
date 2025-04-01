CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -D_POSIX_C_SOURCE=200809L
LDFLAGS = 

SRCS = token.c lexer.c ast.c parser.c symbol_table.c code_gen.c compiler.c debug.c main.c
OBJS = $(SRCS:.c=.o)
HEADERS = $(SRCS:.c=.h)
TARGET = ../bin/sn

.PHONY: all clean

all: create-bin-dir $(TARGET)

create-bin-dir:
	@mkdir -p ../bin

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@chmod +x $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) *.o *.asm
	rm -f $(TARGET)