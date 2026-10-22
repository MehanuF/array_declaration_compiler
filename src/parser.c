/*
 * Array Declaration Compiler
 * Member 3 — Recursive-Descent Parser (implementation)
 *
 * Implements a hand-written LL(1) predictive parser for the CFG
 * defined in grammar/array_grammar.txt.  Each grammar non-terminal
 * maps 1-to-1 to a static C function.  Each function builds and
 * returns the corresponding AST subtree.
 *
 * Grammar (for reference):
 *
 *   Program         → StatementList
 *   StatementList   → Statement StatementList | ε
 *   Statement       → Declaration ; | Assignment ;
 *   Declaration     → Type ID [ INT_LITERAL ] InitializerOpt
 *   Type            → int | float | char
 *   InitializerOpt  → = { InitializerList } | ε
 *   InitializerList → Expression InitializerTail
 *   InitializerTail → , Expression InitializerTail | ε
 *   Assignment      → LValue = Expression
 *   LValue          → ID | ID [ Expression ]
 *   Expression      → Term ExpressionTail
 *   ExpressionTail  → + Term ExpressionTail | - Term ExpressionTail | ε
 *   Term            → Factor TermTail
 *   TermTail        → * Factor TermTail | / Factor TermTail | ε
 *   Factor          → ID | INT_LITERAL | ( Expression ) | ID [ Expression ]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

/* ── Parser state ─────────────────────────────────────────────── */
typedef struct {
    const Token *tokens;
    int          numTokens;
    int          pos;        /* index of the current token         */
    int          hadError;   /* set to 1 on first syntax error     */
} Parser;

/* ── Forward declarations of grammar functions ────────────────── */
static ASTNode *parseStatementList(Parser *p);
static ASTNode *parseStatement    (Parser *p);
static ASTNode *parseDeclaration  (Parser *p);
static ASTNode *parseInitializerOpt(Parser *p);
static ASTNode *parseInitializerList(Parser *p);
static ASTNode *parseAssignment   (Parser *p);
static ASTNode *parseLValue       (Parser *p);
static ASTNode *parseExpression   (Parser *p);
static ASTNode *parseExpressionTail(Parser *p, ASTNode *left);
static ASTNode *parseTerm         (Parser *p);
static ASTNode *parseTermTail     (Parser *p, ASTNode *left);
static ASTNode *parseFactor       (Parser *p);

/* ── Utility: current token ───────────────────────────────────── */
static const Token *current(const Parser *p) {
    if (p->pos < p->numTokens) return &p->tokens[p->pos];
    return &p->tokens[p->numTokens - 1]; /* last token = EOF */
}

/* ── Utility: peek ahead by `n` positions ────────────────────── */
static const Token *peek(const Parser *p, int n) {
    int idx = p->pos + n;
    if (idx >= p->numTokens) return &p->tokens[p->numTokens - 1];
    return &p->tokens[idx];
}

/* ── Utility: advance and return consumed token ──────────────── */
static const Token *advance(Parser *p) {
    const Token *t = current(p);
    if (p->pos < p->numTokens - 1) p->pos++;
    return t;
}

/* ── Utility: consume expected token or report error ─────────── */
static const Token *expect(Parser *p, TokenType type) {
    const Token *t = current(p);
    if (t->type != type) {
        fprintf(stderr,
            "Syntax Error [line %d, col %d]: expected '%s', found '%s' ('%s')\n",
            t->line, t->column,
            tokenName(type), tokenName(t->type), t->lexeme);
        p->hadError = 1;
        return t; /* continue to collect more errors */
    }
    return advance(p);
}

/* ── isType ───────────────────────────────────────────────────── */
static int isType(TokenType t) {
    return t == TOK_INT || t == TOK_FLOAT || t == TOK_CHAR;
}

/* ================================================================
   GRAMMAR FUNCTIONS
   ================================================================ */

/*
 * Program → StatementList
 */
static ASTNode *parseProgram(Parser *p) {
    const Token *t  = current(p);
    ASTNode     *root = makeNode(NODE_PROGRAM, t->line, t->column);
    ASTNode     *list = parseStatementList(p);

    /* Add each statement as a direct child of PROGRAM */
    for (int i = 0; i < list->numChildren; i++)
        addChild(root, list->children[i]);

    /* Free the temporary list node (not its children) */
    free(list);
    return root;
}

/*
 * StatementList → Statement StatementList | ε
 */
static ASTNode *parseStatementList(Parser *p) {
    const Token *t    = current(p);
    ASTNode     *list = makeNode(NODE_PROGRAM, t->line, t->column);

    while (!p->hadError) {
        TokenType tt = current(p)->type;

        /* FIRST(Statement) = {int, float, char, ID} */
        if (!isType(tt) && tt != TOK_ID && tt != TOK_EOF)
            break;
        if (tt == TOK_EOF)
            break;

        ASTNode *stmt = parseStatement(p);
        if (stmt) addChild(list, stmt);
    }
    return list;
}

/*
 * Statement → Declaration ; | Assignment ;
 *
 * Disambiguate: if the first token is a type keyword → Declaration.
 * Otherwise start with ID → Assignment.
 */
static ASTNode *parseStatement(Parser *p) {
    if (isType(current(p)->type)) {
        ASTNode *decl = parseDeclaration(p);
        expect(p, TOK_SEMICOLON);
        return decl;
    }
    if (current(p)->type == TOK_ID) {
        ASTNode *asgn = parseAssignment(p);
        expect(p, TOK_SEMICOLON);
        return asgn;
    }
    /* Unexpected token */
    const Token *t = current(p);
    fprintf(stderr,
        "Syntax Error [line %d, col %d]: unexpected token '%s' ('%s')\n",
        t->line, t->column, tokenName(t->type), t->lexeme);
    p->hadError = 1;
    return NULL;
}

/*
 * Declaration → Type ID [ INT_LITERAL ] InitializerOpt
 */
static ASTNode *parseDeclaration(Parser *p) {
    const Token *typeTok = current(p);
    ASTNode     *node    = makeNode(NODE_DECL, typeTok->line, typeTok->column);

    /* Type */
    if (!isType(typeTok->type)) {
        fprintf(stderr,
            "Syntax Error [line %d, col %d]: expected type keyword, found '%s'\n",
            typeTok->line, typeTok->column, typeTok->lexeme);
        p->hadError = 1;
        return node;
    }
    strncpy(node->typeName, typeTok->lexeme, sizeof(node->typeName) - 1);
    advance(p);

    /* ID */
    const Token *idTok = expect(p, TOK_ID);
    strncpy(node->name, idTok->lexeme, sizeof(node->name) - 1);

    /* [ */
    expect(p, TOK_LBRACKET);

    /* INT_LITERAL (array size) */
    const Token *sizeTok = current(p);
    if (sizeTok->type != TOK_INT_LITERAL) {
        fprintf(stderr,
            "Syntax Error [line %d, col %d]: expected array size (integer), found '%s'\n",
            sizeTok->line, sizeTok->column, sizeTok->lexeme);
        p->hadError = 1;
    } else {
        node->intVal = atoi(sizeTok->lexeme);
        advance(p);
    }

    /* ] */
    expect(p, TOK_RBRACKET);

    /* OptInitializer */
    ASTNode *init = parseInitializerOpt(p);
    if (init) addChild(node, init);

    return node;
}

/*
 * InitializerOpt → = { InitializerList } | ε
 */
static ASTNode *parseInitializerOpt(Parser *p) {
    if (current(p)->type != TOK_ASSIGN) return NULL; /* ε */

    const Token *eq = advance(p); /* consume '=' */
    expect(p, TOK_LBRACE);

    ASTNode *list = makeNode(NODE_INIT_LIST, eq->line, eq->column);
    ASTNode *inner = parseInitializerList(p);

    for (int i = 0; i < inner->numChildren; i++)
        addChild(list, inner->children[i]);
    free(inner);

    expect(p, TOK_RBRACE);
    return list;
}

/*
 * InitializerList → Expression InitializerTail
 */
static ASTNode *parseInitializerList(Parser *p) {
    const Token *t    = current(p);
    ASTNode     *list = makeNode(NODE_INIT_LIST, t->line, t->column);

    ASTNode *expr = parseExpression(p);
    if (expr) addChild(list, expr);

    /* InitializerTail → , Expression InitializerTail | ε */
    while (current(p)->type == TOK_COMMA) {
        advance(p); /* consume ',' */
        ASTNode *e = parseExpression(p);
        if (e) addChild(list, e);
    }
    return list;
}

/*
 * Assignment → LValue = Expression
 */
static ASTNode *parseAssignment(Parser *p) {
    const Token *t    = current(p);
    ASTNode     *node = makeNode(NODE_ASSIGN, t->line, t->column);

    ASTNode *lval = parseLValue(p);
    if (lval) addChild(node, lval);

    expect(p, TOK_ASSIGN);

    ASTNode *expr = parseExpression(p);
    if (expr) addChild(node, expr);

    return node;
}

/*
 * LValue → ID | ID [ Expression ]
 *
 * Both alternatives start with ID.  Look at the next token to
 * decide: if it's '[' we have an indexed access.
 */
static ASTNode *parseLValue(Parser *p) {
    const Token *idTok = expect(p, TOK_ID);

    if (current(p)->type == TOK_LBRACKET) {
        /* ID [ Expression ] */
        ASTNode *node = makeNode(NODE_ARRAY_ACCESS,
                                 idTok->line, idTok->column);
        strncpy(node->name, idTok->lexeme, sizeof(node->name) - 1);
        advance(p); /* consume '[' */
        ASTNode *idx = parseExpression(p);
        if (idx) addChild(node, idx);
        expect(p, TOK_RBRACKET);
        return node;
    }

    /* Plain identifier */
    ASTNode *node = makeNode(NODE_ID, idTok->line, idTok->column);
    strncpy(node->name, idTok->lexeme, sizeof(node->name) - 1);
    return node;
}

/*
 * Expression → Term ExpressionTail
 */
static ASTNode *parseExpression(Parser *p) {
    ASTNode *left = parseTerm(p);
    return parseExpressionTail(p, left);
}

/*
 * ExpressionTail → + Term ExpressionTail
 *                | - Term ExpressionTail
 *                | ε
 */
static ASTNode *parseExpressionTail(Parser *p, ASTNode *left) {
    TokenType tt = current(p)->type;
    if (tt != TOK_PLUS && tt != TOK_MINUS) return left; /* ε */

    const Token *opTok = advance(p);
    ASTNode     *node  = makeNode(NODE_BINOP, opTok->line, opTok->column);
    node->name[0] = opTok->lexeme[0];
    node->name[1] = '\0';

    ASTNode *right = parseTerm(p);
    addChild(node, left);
    addChild(node, right);

    return parseExpressionTail(p, node);
}

/*
 * Term → Factor TermTail
 */
static ASTNode *parseTerm(Parser *p) {
    ASTNode *left = parseFactor(p);
    return parseTermTail(p, left);
}

/*
 * TermTail → * Factor TermTail | / Factor TermTail | ε
 */
static ASTNode *parseTermTail(Parser *p, ASTNode *left) {
    TokenType tt = current(p)->type;
    if (tt != TOK_MULTIPLY && tt != TOK_DIVIDE) return left; /* ε */

    const Token *opTok = advance(p);
    ASTNode     *node  = makeNode(NODE_BINOP, opTok->line, opTok->column);
    node->name[0] = opTok->lexeme[0];
    node->name[1] = '\0';

    ASTNode *right = parseFactor(p);
    addChild(node, left);
    addChild(node, right);

    return parseTermTail(p, node);
}

/*
 * Factor → ID
 *         | INT_LITERAL
 *         | ( Expression )
 *         | ID [ Expression ]
 *
 * Both ID alternatives start with TOK_ID; look at token+1 to pick.
 */
static ASTNode *parseFactor(Parser *p) {
    const Token *t = current(p);

    if (t->type == TOK_ID) {
        if (peek(p, 1)->type == TOK_LBRACKET) {
            /* ID [ Expression ] */
            const Token *idTok = advance(p); /* ID */
            ASTNode     *node  = makeNode(NODE_ARRAY_ACCESS,
                                          idTok->line, idTok->column);
            strncpy(node->name, idTok->lexeme, sizeof(node->name) - 1);
            advance(p); /* '[' */
            ASTNode *idx = parseExpression(p);
            if (idx) addChild(node, idx);
            expect(p, TOK_RBRACKET);
            return node;
        }
        /* Plain ID */
        advance(p);
        ASTNode *node = makeNode(NODE_ID, t->line, t->column);
        strncpy(node->name, t->lexeme, sizeof(node->name) - 1);
        return node;
    }

    if (t->type == TOK_INT_LITERAL) {
        advance(p);
        ASTNode *node = makeNode(NODE_INT_LIT, t->line, t->column);
        node->intVal = atoi(t->lexeme);
        return node;
    }

    if (t->type == TOK_LPAREN) {
        advance(p); /* '(' */
        ASTNode *expr = parseExpression(p);
        expect(p, TOK_RPAREN);
        return expr;
    }

    /* Error */
    fprintf(stderr,
        "Syntax Error [line %d, col %d]: expected expression, found '%s' ('%s')\n",
        t->line, t->column, tokenName(t->type), t->lexeme);
    p->hadError = 1;

    ASTNode *err = makeNode(NODE_INT_LIT, t->line, t->column);
    err->intVal = 0;
    return err;
}

/* ================================================================
   PUBLIC ENTRY POINT
   ================================================================ */

ASTNode *parse(const Token *tokens, int numTokens) {
    Parser p;
    p.tokens    = tokens;
    p.numTokens = numTokens;
    p.pos       = 0;
    p.hadError  = 0;

    ASTNode *root = parseProgram(&p);

    /* Check we consumed everything (except EOF) */
    if (!p.hadError && current(&p)->type != TOK_EOF) {
        const Token *t = current(&p);
        fprintf(stderr,
            "Syntax Error [line %d, col %d]: unexpected token '%s' after end of input\n",
            t->line, t->column, t->lexeme);
        p.hadError = 1;
    }

    if (p.hadError) {
        freeAST(root);
        return NULL;
    }
    return root;
}
