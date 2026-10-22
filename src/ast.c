/*
 * Array Declaration Compiler
 * Member 3 — Abstract Syntax Tree (implementation)
 *
 * Provides node allocation, child linking, tree printing, and
 * recursive deallocation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

/* ── makeNode ─────────────────────────────────────────────────── */
ASTNode *makeNode(NodeKind kind, int line, int col) {
    ASTNode *n = (ASTNode *)calloc(1, sizeof(ASTNode));
    if (!n) {
        fprintf(stderr, "Fatal: out of memory\n");
        exit(1);
    }
    n->kind        = kind;
    n->line        = line;
    n->column      = col;
    n->numChildren = 0;
    return n;
}

/* ── addChild ─────────────────────────────────────────────────── */
void addChild(ASTNode *parent, ASTNode *child) {
    if (!parent || !child) return;
    if (parent->numChildren >= MAX_CHILDREN) {
        fprintf(stderr, "Fatal: too many children for AST node\n");
        exit(1);
    }
    parent->children[parent->numChildren++] = child;
}

/* ── freeAST ──────────────────────────────────────────────────── */
void freeAST(ASTNode *node) {
    if (!node) return;
    for (int i = 0; i < node->numChildren; i++)
        freeAST(node->children[i]);
    free(node);
}

/* ── internal: kind → label string ───────────────────────────── */
static const char *kindLabel(NodeKind k) {
    switch (k) {
        case NODE_PROGRAM:      return "PROGRAM";
        case NODE_DECL:         return "DECL";
        case NODE_ASSIGN:       return "ASSIGN";
        case NODE_INIT_LIST:    return "INIT_LIST";
        case NODE_BINOP:        return "BINOP";
        case NODE_ID:           return "ID";
        case NODE_INT_LIT:      return "INT_LIT";
        case NODE_ARRAY_ACCESS: return "ARRAY_ACCESS";
        default:                return "UNKNOWN";
    }
}

/* ── printAST ─────────────────────────────────────────────────── */
void printAST(const ASTNode *node, int depth) {
    if (!node) return;

    /* Indentation: 4 spaces per level, with a tree connector */
    for (int i = 0; i < depth; i++) {
        if (i == depth - 1)
            printf("  |-- ");
        else
            printf("  |   ");
    }
    if (depth == 0) printf("  ");

    /* Node label */
    printf("[%s]", kindLabel(node->kind));

    /* Extra payload */
    switch (node->kind) {
        case NODE_DECL:
            printf("  type=%s  name=%s  size=%d",
                   node->typeName, node->name, node->intVal);
            break;
        case NODE_ID:
            printf("  \"%s\"", node->name);
            break;
        case NODE_INT_LIT:
            printf("  %d", node->intVal);
            break;
        case NODE_BINOP:
            printf("  op='%s'", node->name);
            break;
        case NODE_ARRAY_ACCESS:
            printf("  name=%s", node->name);
            break;
        case NODE_ASSIGN:
        case NODE_INIT_LIST:
        case NODE_PROGRAM:
        default:
            break;
    }

    printf("  (line %d, col %d)\n", node->line, node->column);

    for (int i = 0; i < node->numChildren; i++)
        printAST(node->children[i], depth + 1);
}
