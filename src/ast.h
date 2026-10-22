/*
 * Array Declaration Compiler
 * Member 3 — Abstract Syntax Tree (header)
 *
 * Defines every AST node type produced by the recursive-descent
 * parser.  A single ASTNode union covers all syntactic forms so
 * that the semantic analyzer can walk the tree uniformly.
 */

#ifndef AST_H
#define AST_H

/* Maximum number of children an AST node can hold */
#define MAX_CHILDREN 64

/* ── Node kinds ───────────────────────────────────────────────── */
typedef enum {
    /* Top-level */
    NODE_PROGRAM,          /* root: list of statements              */
    /* Statements */
    NODE_DECL,             /* Type ID [ size ] OptInit              */
    NODE_ASSIGN,           /* LValue = Expr                         */
    /* Sub-parts of declaration */
    NODE_INIT_LIST,        /* { e1, e2, … }                         */
    /* Expressions */
    NODE_BINOP,            /* e1 op e2                              */
    NODE_ID,               /* identifier leaf                       */
    NODE_INT_LIT,          /* integer literal leaf                  */
    NODE_ARRAY_ACCESS,     /* id [ expr ]                           */
} NodeKind;

/* ── ASTNode ─────────────────────────────────────────────────── */
typedef struct ASTNode {
    NodeKind kind;

    /* Payload — which fields are used depends on kind */
    char     name[64];      /* NODE_ID, NODE_DECL (array name), NODE_BINOP (op) */
    char     typeName[16];  /* NODE_DECL: "int" / "float" / "char"             */
    int      intVal;        /* NODE_INT_LIT, NODE_DECL (size)                  */

    /* Source location */
    int      line;
    int      column;

    /* Children */
    struct ASTNode *children[MAX_CHILDREN];
    int             numChildren;
} ASTNode;

/* ── AST construction helpers ────────────────────────────────── */
ASTNode *makeNode(NodeKind kind, int line, int col);
void     addChild(ASTNode *parent, ASTNode *child);
void     freeAST(ASTNode *node);

/* ── Pretty-printer ──────────────────────────────────────────── */
void printAST(const ASTNode *node, int depth);

#endif /* AST_H */
