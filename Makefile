CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -Iinclude
COMMON_SOURCES = src/compiler.c src/token.c src/lexer.c src/ast.c src/parser.c

.PHONY: all test clean

all: mini_compiler

mini_compiler: $(COMMON_SOURCES) src/main.c
	$(CC) $(CFLAGS) $^ -o $@

compiler_tests: $(COMMON_SOURCES) tests/test_compiler.c
	$(CC) $(CFLAGS) $^ -o $@

test: compiler_tests
	./compiler_tests

clean:
	$(RM) mini_compiler compiler_tests mini_compiler.exe compiler_tests.exe
