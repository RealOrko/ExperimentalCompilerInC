CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer -no-pie -MMD -MP
LDFLAGS = -fsanitize=address -fsanitize=leak -no-pie

SRCDIR = .
SRCS = token.c lexer.c ast.c parser.c symbol_table.c code_gen.c compiler.c debug.c main.c
HEADERS = token.h lexer.h ast.h parser.h symbol_table.h code_gen.h compiler.h debug.h
BIN_DIR = ../bin
OBJS = $(addprefix $(BIN_DIR)/, $(notdir $(SRCS:.c=.o)))
DEPS = $(OBJS:.o=.d)
TARGET = $(BIN_DIR)/sn

VPATH = $(SRCDIR)

.PHONY: all clean

all: create-bin-dir $(TARGET)

create-bin-dir:
	@mkdir -p $(BIN_DIR)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@chmod +x $@

$(BIN_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(BIN_DIR)/*.o $(BIN_DIR)/*.d *.asm
	rm -f $(TARGET)
