CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -g
INCLUDES = -Iinclude -Isrc

TARGET = array_compiler
TEST_TARGET = compiler_tests

SRCS = src/compiler.c \
       src/token.c \
       src/lexer.c \
       src/ast.c \
       src/parser.c \
       src/semantic.c \
       src/main.c

OBJS = $(SRCS:.c=.o)
TEST_SRCS = src/compiler.c \
            src/token.c \
            src/lexer.c \
            src/ast.c \
            src/parser.c \
            src/semantic.c \
            tests/test_compiler.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(TEST_TARGET): $(TEST_SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(TEST_SRCS)

test: $(TARGET) $(TEST_TARGET)
	./$(TEST_TARGET)
	@set -e; \
	failed=0; \
	for f in tests/inputs/v*.c; do \
		if ! ./$(TARGET) $$f >/dev/null 2>&1; then \
			echo "FAIL: expected ACCEPTED for $$f"; failed=1; \
		fi; \
	done; \
	for f in tests/inputs/i*.c tests/inputs/s*.c; do \
		if ./$(TARGET) $$f >/dev/null 2>&1; then \
			echo "FAIL: expected REJECTED for $$f"; failed=1; \
		fi; \
	done; \
	if [ $$failed -ne 0 ]; then exit 1; fi

clean:
	rm -f $(TARGET) $(TEST_TARGET) src/*.o
