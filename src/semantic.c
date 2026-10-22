/*
 * Array Declaration Compiler
 * Member 4 — Semantic Analyzer (implementation)
 *
 * Enforces all six semantic rules documented in array_grammar.txt:
 *
 *  Rule 1. Array must be declared before use.
 *  Rule 2. Array index must have integer type.
 *  Rule 3. Array index must be within declared bounds (literal indices).
 *  Rule 4. Assigned value must be compatible with the array element type.
 *  Rule 5. Initializer count must not exceed declared array size.
 *  Rule 6. Array size must be a positive integer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h"

/* ── ElemType helpers ─────────────────────────────────────────── */
static ElemType strToType(const char *s) {
    if (strcmp(s, "int")   == 0) return TYPE_INT;
    if (strcmp(s, "float") == 0) return TYPE_FLOAT;
    if (strcmp(s, "char")  == 0) return TYPE_CHAR;
    return TYPE_UNKNOWN;
}

static const char *typeToStr(ElemType t) {
    switch (t) {
        case TYPE_INT:   return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_CHAR:  return "char";
        default:         return "unknown";
    }
}

/* ── Symbol table operations ──────────────────────────────────── */
static Symbol *lookupSymbol(SymbolTable *st, const char *name) {
    for (int i = 0; i < st->count; i++)
        if (strcmp(st->entries[i].name, name) == 0)
            return &st->entries[i];
    return NULL;
}

static int insertSymbol(SymbolTable *st, const char *name,
                        ElemType elemType, int size, int line) {
    if (st->count >= MAX_SYMBOLS) {
        fprintf(stderr, "Semantic Error: symbol table full\n");
        return -1;
    }
    Symbol *s = &st->entries[st->count++];
    strncpy(s->name, name, sizeof(s->name) - 1);
    s->name[sizeof(s->name) - 1] = '\0';
    s->elemType = elemType;
    s->size     = size;
    s->declLine = line;
    return 0;
}

/* ── Expression type inference (simplified) ───────────────────── */
/*
 * For the purposes of this compiler all integer literals produce
 * TYPE_INT.  All arithmetic between integers stays TYPE_INT.
 * Identifiers and array accesses inherit the declared element type.
 * (Float and char arithmetic would need a wider type system.)
 */
typedef struct {
    ElemType type;      /* inferred type of the sub-expression     */
    int      isLiteral; /* 1 if the expression is an INT_LITERAL   */
    int      litVal;    /* value if isLiteral == 1                  */
} ExprResult;

static ExprResult analyzeExpr(const ASTNode *node,
                               SymbolTable   *st,
                               int           *errorCount);

static ExprResult analyzeExpr(const ASTNode *node,
                               SymbolTable   *st,
                               int           *errorCount) {
    ExprResult r = {TYPE_UNKNOWN, 0, 0};
    if (!node) return r;

    switch (node->kind) {
        case NODE_INT_LIT:
            r.type      = TYPE_INT;
            r.isLiteral = 1;
            r.litVal    = node->intVal;
            break;

        case NODE_ID: {
            Symbol *sym = lookupSymbol(st, node->name);
            if (!sym) {
                fprintf(stderr,
                    "Semantic Error [line %d, col %d]: "
                    "undeclared identifier '%s'\n",
                    node->line, node->column, node->name);
                (*errorCount)++;
                r.type = TYPE_UNKNOWN;
            } else {
                r.type = sym->elemType;
            }
            break;
        }

        case NODE_ARRAY_ACCESS: {
            /* Rule 1: array must be declared */
            Symbol *sym = lookupSymbol(st, node->name);
            if (!sym) {
                fprintf(stderr,
                    "Semantic Error [line %d, col %d]: "
                    "undeclared array '%s'\n",
                    node->line, node->column, node->name);
                (*errorCount)++;
                r.type = TYPE_UNKNOWN;
                break;
            }
            r.type = sym->elemType;

            /* Analyze the index expression */
            if (node->numChildren > 0) {
                ExprResult idx = analyzeExpr(node->children[0], st, errorCount);

                /* Rule 2: index must be integer type */
                if (idx.type != TYPE_INT && idx.type != TYPE_UNKNOWN) {
                    fprintf(stderr,
                        "Semantic Error [line %d, col %d]: "
                        "array index for '%s' must be of integer type, found '%s'\n",
                        node->line, node->column,
                        node->name, typeToStr(idx.type));
                    (*errorCount)++;
                }

                /* Rule 3: literal index must be within bounds */
                if (idx.isLiteral) {
                    if (idx.litVal < 0 || idx.litVal >= sym->size) {
                        fprintf(stderr,
                            "Semantic Error [line %d, col %d]: "
                            "index %d is out of bounds for array '%s' "
                            "(declared size %d)\n",
                            node->line, node->column,
                            idx.litVal, node->name, sym->size);
                        (*errorCount)++;
                    }
                }
            }
            break;
        }

        case NODE_BINOP: {
            ExprResult l = {TYPE_UNKNOWN, 0, 0};
            ExprResult rr = {TYPE_UNKNOWN, 0, 0};
            if (node->numChildren >= 1)
                l  = analyzeExpr(node->children[0], st, errorCount);
            if (node->numChildren >= 2)
                rr = analyzeExpr(node->children[1], st, errorCount);

            /* Result type: promote to widest */
            if (l.type == TYPE_FLOAT || rr.type == TYPE_FLOAT)
                r.type = TYPE_FLOAT;
            else if (l.type == TYPE_INT || rr.type == TYPE_INT)
                r.type = TYPE_INT;
            else
                r.type = TYPE_UNKNOWN;
            break;
        }

        default:
            break;
    }
    return r;
}

/* ── Analyze a DECL node ──────────────────────────────────────── */
static void analyzeDecl(const ASTNode *node,
                         SymbolTable   *st,
                         int           *errorCount) {
    /* Rule 6: size must be positive */
    if (node->intVal <= 0) {
        fprintf(stderr,
            "Semantic Error [line %d, col %d]: "
            "array '%s' declared with non-positive size %d\n",
            node->line, node->column, node->name, node->intVal);
        (*errorCount)++;
    }

    ElemType et = strToType(node->typeName);

    /* Register array (allow redeclaration with a warning for simplicity) */
    if (lookupSymbol(st, node->name)) {
        fprintf(stderr,
            "Semantic Warning [line %d, col %d]: "
            "array '%s' already declared (redeclaration ignored)\n",
            node->line, node->column, node->name);
    } else {
        insertSymbol(st, node->name, et, node->intVal, node->line);
    }

    /* Rule 5: initializer count must not exceed declared size */
    if (node->numChildren > 0) {
        const ASTNode *initList = node->children[0]; /* NODE_INIT_LIST */
        int initCount = initList->numChildren;
        if (initCount > node->intVal) {
            fprintf(stderr,
                "Semantic Error [line %d, col %d]: "
                "array '%s' declared with size %d but has %d initializers\n",
                node->line, node->column,
                node->name, node->intVal, initCount);
            (*errorCount)++;
        }

        /* Rule 4: type-check each initializer expression */
        for (int i = 0; i < initList->numChildren; i++) {
            ExprResult er = analyzeExpr(initList->children[i], st, errorCount);
            if (er.type != TYPE_UNKNOWN && er.type != et) {
                /* char arrays can accept int literals (common C practice) */
                if (!(et == TYPE_CHAR && er.type == TYPE_INT)) {
                    fprintf(stderr,
                        "Semantic Error [line %d, col %d]: "
                        "type mismatch in initializer %d of '%s' "
                        "(expected '%s', got '%s')\n",
                        initList->children[i]->line,
                        initList->children[i]->column,
                        i + 1, node->name,
                        typeToStr(et), typeToStr(er.type));
                    (*errorCount)++;
                }
            }
        }
    }
}

/* ── Analyze an ASSIGN node ───────────────────────────────────── */
static void analyzeAssign(const ASTNode *node,
                           SymbolTable   *st,
                           int           *errorCount) {
    if (node->numChildren < 2) return;

    const ASTNode *lval = node->children[0];
    const ASTNode *rval = node->children[1];

    /* Get the type of the LValue */
    ElemType ltype = TYPE_UNKNOWN;

    if (lval->kind == NODE_ARRAY_ACCESS) {
        /* Rule 1 + 2 + 3 handled inside analyzeExpr */
        ExprResult lr = analyzeExpr(lval, st, errorCount);
        ltype = lr.type;
    } else if (lval->kind == NODE_ID) {
        Symbol *sym = lookupSymbol(st, lval->name);
        if (!sym) {
            fprintf(stderr,
                "Semantic Error [line %d, col %d]: "
                "undeclared identifier '%s'\n",
                lval->line, lval->column, lval->name);
            (*errorCount)++;
        } else {
            ltype = sym->elemType;
        }
    }

    /* Analyze RHS */
    ExprResult rr = analyzeExpr(rval, st, errorCount);

    /* Rule 4: type compatibility */
    if (ltype != TYPE_UNKNOWN && rr.type != TYPE_UNKNOWN
        && ltype != rr.type) {
        /* Allow int → float widening */
        if (!(ltype == TYPE_FLOAT && rr.type == TYPE_INT)) {
            fprintf(stderr,
                "Semantic Warning [line %d, col %d]: "
                "possible type mismatch in assignment "
                "(lvalue is '%s', rvalue is '%s')\n",
                node->line, node->column,
                typeToStr(ltype), typeToStr(rr.type));
        }
    }
}

/* ── Walk the full AST ────────────────────────────────────────── */
static void walkAST(const ASTNode *node,
                    SymbolTable   *st,
                    int           *errorCount) {
    if (!node) return;

    switch (node->kind) {
        case NODE_PROGRAM:
            for (int i = 0; i < node->numChildren; i++)
                walkAST(node->children[i], st, errorCount);
            break;

        case NODE_DECL:
            analyzeDecl(node, st, errorCount);
            break;

        case NODE_ASSIGN:
            analyzeAssign(node, st, errorCount);
            break;

        default:
            /* Expression nodes are handled inside analyzeExpr */
            break;
    }
}

/* ── Public entry point ───────────────────────────────────────── */
int analyzeAST(const ASTNode *root) {
    SymbolTable st;
    memset(&st, 0, sizeof(st));

    int errorCount = 0;
    walkAST(root, &st, &errorCount);
    return errorCount;
}
