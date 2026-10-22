# Array Declaration Compiler — Makefile

CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c99 -g
SRCDIR  = src
TARGET  = array_compiler

# Our compiler front-end sources (Member 2, 3, 4)
SRCS = $(SRCDIR)/main.c \
       $(SRCDIR)/lexer.c \
       $(SRCDIR)/ast.c \
       $(SRCDIR)/parser.c \
       $(SRCDIR)/semantic.c

OBJS = $(SRCS:.c=.o)

# ── Default target ─────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile each .c to .o, headers live in src/ only
$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I$(SRCDIR) -c -o $@ $<

# ── Run all tests ──────────────────────────────────────────────
.PHONY: test
test: $(TARGET)
	@echo ""
	@echo "======================================================"
	@echo "  VALID SYNTAX TESTS (expect: ACCEPTED)"
	@echo "======================================================"
	@for f in tests/inputs/v*.c; do \
	    echo ""; \
	    echo "--- $$f ---"; \
	    ./$(TARGET) $$f; \
	done
	@echo ""
	@echo "======================================================"
	@echo "  INVALID SYNTAX TESTS (expect: REJECTED)"
	@echo "======================================================"
	@for f in tests/inputs/i*.c; do \
	    echo ""; \
	    echo "--- $$f ---"; \
	    ./$(TARGET) $$f; \
	done
	@echo ""
	@echo "======================================================"
	@echo "  SEMANTIC ERROR TESTS (expect: REJECTED)"
	@echo "======================================================"
	@for f in tests/inputs/s*.c; do \
	    echo ""; \
	    echo "--- $$f ---"; \
	    ./$(TARGET) $$f; \
	done

# ── Clean ──────────────────────────────────────────────────────
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
