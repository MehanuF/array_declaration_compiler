# Array Declaration Compiler — Makefile

CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c99 -g -Isrc
TARGET  = array_compiler

SRCS = src/main.c src/lexer.c src/ast.c src/parser.c src/semantic.c
OBJS = $(SRCS:.c=.o)

# ── Default target ─────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# ── Automated test runner (verifies exit codes) ────────────────
.PHONY: test
test: $(TARGET)
	@pass=0; fail=0; total=0; \
	echo ""; \
	echo "======================================================"; \
	echo "  VALID SYNTAX TESTS (expect exit 0 = ACCEPTED)"; \
	echo "======================================================"; \
	for f in tests/inputs/v*.c; do \
	    total=$$((total + 1)); \
	    ./$(TARGET) $$f > /dev/null 2>&1; \
	    rc=$$?; \
	    if [ $$rc -eq 0 ]; then \
	        echo "  PASS  $$f"; \
	        pass=$$((pass + 1)); \
	    else \
	        echo "  FAIL  $$f  (exit $$rc, expected 0)"; \
	        fail=$$((fail + 1)); \
	    fi; \
	done; \
	echo ""; \
	echo "======================================================"; \
	echo "  INVALID SYNTAX TESTS (expect exit 1 = REJECTED)"; \
	echo "======================================================"; \
	for f in tests/inputs/i*.c; do \
	    total=$$((total + 1)); \
	    ./$(TARGET) $$f > /dev/null 2>&1; \
	    rc=$$?; \
	    if [ $$rc -eq 1 ]; then \
	        echo "  PASS  $$f"; \
	        pass=$$((pass + 1)); \
	    else \
	        echo "  FAIL  $$f  (exit $$rc, expected 1)"; \
	        fail=$$((fail + 1)); \
	    fi; \
	done; \
	echo ""; \
	echo "======================================================"; \
	echo "  SEMANTIC ERROR TESTS (expect exit 2 = REJECTED)"; \
	echo "======================================================"; \
	for f in tests/inputs/s*.c; do \
	    total=$$((total + 1)); \
	    ./$(TARGET) $$f > /dev/null 2>&1; \
	    rc=$$?; \
	    if [ $$rc -eq 2 ]; then \
	        echo "  PASS  $$f"; \
	        pass=$$((pass + 1)); \
	    else \
	        echo "  FAIL  $$f  (exit $$rc, expected 2)"; \
	        fail=$$((fail + 1)); \
	    fi; \
	done; \
	echo ""; \
	echo "======================================================"; \
	echo "  RESULTS: $$pass/$$total passed, $$fail failed"; \
	echo "======================================================"; \
	if [ $$fail -ne 0 ]; then exit 1; fi

# ── Run a single test with full output ─────────────────────────
.PHONY: run
run: $(TARGET)
	@echo "Usage: make run FILE=tests/inputs/v4.c"
	@if [ -n "$(FILE)" ]; then ./$(TARGET) $(FILE); fi

# ── Clean ──────────────────────────────────────────────────────
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
	rm -f lex_yacc/lex.yy.c lex_yacc/parser.tab.c lex_yacc/parser.tab.h
	rm -f array_compiler_ly

# ── Lex / YACC version ─────────────────────────────────────────
# Requires: flex and bison
# Usage:    make lex-yacc
#           ./array_compiler_ly tests/inputs/v4.c
.PHONY: lex-yacc
lex-yacc:
	bison -d -o lex_yacc/parser.tab.c lex_yacc/parser.y
	flex  -o lex_yacc/lex.yy.c         lex_yacc/lexer.l
	$(CC) -Wall -g \
	    lex_yacc/parser.tab.c \
	    lex_yacc/lex.yy.c \
	    -Ilex_yacc \
	    -o array_compiler_ly
	@echo ""
	@echo "Built: array_compiler_ly  (Lex + YACC version)"
	@echo "Run:   ./array_compiler_ly <file.c>"
