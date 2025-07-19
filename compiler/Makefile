CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -fsanitize=address -fno-omit-frame-pointer -MMD -MP
LDFLAGS = -fsanitize=address

SRCDIR = .
SRCS = runtime.c token.c lexer.c ast.c parser.c symbol_table.c code_gen.c compiler.c debug.c type_checker.c main.c
HEADERS = runtime.h token.h lexer.h ast.h parser.h symbol_table.h code_gen.h compiler.h debug.h type_checker.h
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