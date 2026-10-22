/*
 * Array Declaration Compiler
 * Member 2 — Lexical Analyzer (header)
 *
 * Defines token types, the Token struct, and the lexer interface
 * used by the parser and main driver.
 */

#ifndef LEXER_H
#define LEXER_H

#define MAX_LEXEME  100
#define MAX_TOKENS  2048

/* ── Token types ──────────────────────────────────────────────── */
typedef enum {
    TOK_INT,
    TOK_FLOAT,
    TOK_CHAR,
    TOK_ID,
    TOK_INT_LITERAL,
    TOK_ASSIGN,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_SEMICOLON,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_COMMA,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MULTIPLY,
    TOK_DIVIDE,
    TOK_EOF,
    TOK_INVALID
} TokenType;

/* ── Token struct ─────────────────────────────────────────────── */
typedef struct {
    TokenType type;
    char      lexeme[MAX_LEXEME];
    int       line;
    int       column;
} Token;

/* ── Public interface ─────────────────────────────────────────── */

/* Returns a human-readable name for a token type */
const char *tokenName(TokenType type);

/*
 * tokenize() — lex the full source string.
 *
 * Fills `tokens[]` with at most `maxTokens` Token values.
 * Returns the number of tokens produced (including TOK_EOF).
 * On a lexical error the final token has type TOK_INVALID.
 */
int tokenize(const char *source, Token *tokens, int maxTokens);

#endif /* LEXER_H */
