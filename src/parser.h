/*
 * Array Declaration Compiler
 * Member 3 — Recursive-Descent Parser (header)
 *
 * Public interface for the LL(1) parser.  Call parse() with the
 * token array produced by tokenize(); it returns the AST root or
 * NULL on a syntax error.
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"

/*
 * parse() — build an AST from a flat token array.
 *
 * tokens    : array produced by tokenize()
 * numTokens : number of tokens (including TOK_EOF)
 *
 * Returns the root ASTNode (NODE_PROGRAM) on success.
 * Returns NULL on a syntax error (error message printed to stderr).
 */
ASTNode *parse(const Token *tokens, int numTokens);

#endif /* PARSER_H */
