/*
 * Array Declaration Compiler
 * Member 4 — Semantic Analyzer (header)
 *
 * Defines the symbol table and the interface for semantic analysis.
 * The analyzer walks the AST produced by the parser and enforces
 * the six semantic rules listed in the grammar documentation.
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

/* ── Array element types ──────────────────────────────────────── */
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_UNKNOWN
} ElemType;

/* ── Symbol table entry ───────────────────────────────────────── */
#define MAX_SYMBOLS 256

typedef struct {
    char     name[64];   /* array identifier                       */
    ElemType elemType;   /* int / float / char                     */
    int      size;       /* declared size (number of elements)     */
    int      declLine;   /* source line of declaration             */
} Symbol;

typedef struct {
    Symbol entries[MAX_SYMBOLS];
    int    count;
} SymbolTable;

/* ── Public interface ─────────────────────────────────────────── */

/*
 * analyzeAST() — walk the AST and perform semantic checks.
 *
 * Returns 0 if no semantic errors were found, nonzero otherwise.
 * Errors are printed to stderr with line/column information.
 */
int analyzeAST(const ASTNode *root);

#endif /* SEMANTIC_H */
