/*
 * Array Declaration Compiler
 * Main Driver
 *
 * Pipeline:
 *   Source file / stdin
 *      → Lexer   (lexer.c)   → Token array
 *      → Parser  (parser.c)  → AST
 *      → Printer (ast.c)     → Parse-tree output
 *      → Semantic (semantic.c) → Error / OK report
 *
 * Usage:
 *   ./array_compiler [filename]
 *   ./array_compiler          (reads from stdin)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "semantic.h"

#define MAX_SOURCE 65536

/* ── Banner ───────────────────────────────────────────────────── */
static void printBanner(void) {
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║         ARRAY DECLARATION COMPILER  v1.0            ║\n");
    printf("║  Lexer → Syntax Analyzer → AST → Semantic Analyzer  ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n\n");
}

/* ── Read source from a file or stdin ────────────────────────── */
static int readSource(const char *filename, char *buf, int maxLen) {
    FILE *fp = filename ? fopen(filename, "r") : stdin;
    if (!fp) {
        fprintf(stderr, "Error: cannot open '%s'\n", filename);
        return -1;
    }

    int len = 0;
    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        if (len >= maxLen - 1) {
            fprintf(stderr, "Error: source file is too large\n");
            if (filename) fclose(fp);
            return -1;
        }
        buf[len++] = (char)ch;
    }
    buf[len] = '\0';

    if (filename) fclose(fp);
    return len;
}

/* ── Print token table ────────────────────────────────────────── */
static void printTokenTable(const Token *tokens, int count) {
    printf("┌─────────────────────────────────────────────────────┐\n");
    printf("│  PHASE 1 — LEXICAL ANALYSIS (Token Stream)          │\n");
    printf("└─────────────────────────────────────────────────────┘\n");
    printf("  %-16s %-14s %s\n", "TOKEN TYPE", "LEXEME", "LOCATION");
    printf("  %-16s %-14s %s\n",
           "────────────────", "──────────────", "────────────");

    for (int i = 0; i < count; i++) {
        printf("  %-16s %-14s line %d, col %d\n",
               tokenName(tokens[i].type),
               tokens[i].lexeme,
               tokens[i].line,
               tokens[i].column);
    }
    printf("\n");
}

/* ── main ─────────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    printBanner();

    /* ── 1. Read source ──────────────────────────────────────── */
    char *source = (char *)malloc(MAX_SOURCE);
    if (!source) {
        fprintf(stderr, "Fatal: out of memory\n");
        return 1;
    }

    const char *filename = (argc >= 2) ? argv[1] : NULL;
    if (!filename)
        printf("Reading from stdin (press Ctrl+D when done):\n\n");

    if (readSource(filename, source, MAX_SOURCE) < 0) {
        free(source);
        return 1;
    }

    /* ── 2. Lexical Analysis ─────────────────────────────────── */
    Token *tokens = (Token *)malloc(MAX_TOKENS * sizeof(Token));
    if (!tokens) {
        fprintf(stderr, "Fatal: out of memory\n");
        free(source);
        return 1;
    }

    int numTokens = tokenize(source, tokens, MAX_TOKENS);
    printTokenTable(tokens, numTokens);

    /* Check for lexical error */
    if (numTokens > 0 && tokens[numTokens - 1].type == TOK_INVALID) {
        printf("Result: REJECTED (lexical error)\n");
        free(tokens);
        free(source);
        return 1;
    }

    /* ── 3. Syntax Analysis (Parsing) ────────────────────────── */
    printf("┌─────────────────────────────────────────────────────┐\n");
    printf("│  PHASE 2 — SYNTAX ANALYZER (Parse Tree / AST)       │\n");
    printf("└─────────────────────────────────────────────────────┘\n");

    ASTNode *root = parse(tokens, numTokens);

    if (!root) {
        printf("\nResult: REJECTED (syntax error — see messages above)\n");
        free(tokens);
        free(source);
        return 1;
    }

    printAST(root, 0);
    printf("\n");

    /* ── 4. Semantic Analysis ────────────────────────────────── */
    printf("┌─────────────────────────────────────────────────────┐\n");
    printf("│  PHASE 3 — SEMANTIC ANALYSIS                        │\n");
    printf("└─────────────────────────────────────────────────────┘\n");

    int errors = analyzeAST(root);

    if (errors == 0) {
        printf("  Semantic analysis: OK — no errors found.\n\n");
        printf("Result: ACCEPTED\n");
    } else {
        printf("\n  Semantic analysis: FAILED — %d error(s) found.\n\n", errors);
        printf("Result: REJECTED (semantic error)\n");
    }

    /* ── Cleanup ─────────────────────────────────────────────── */
    freeAST(root);
    free(tokens);
    free(source);

    return (errors == 0) ? 0 : 2;
}
