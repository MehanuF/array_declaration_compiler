/*
 * Array Declaration Compiler
 * Member 2 — Lexical Analyzer (implementation)
 *
 * Converts a raw source string into a flat token array consumed by
 * the recursive-descent parser.  Error recovery: on an unknown
 * character the scanner emits TOK_INVALID and stops.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h"

/* ── tokenName ────────────────────────────────────────────────── */
const char *tokenName(TokenType type) {
    switch (type) {
        case TOK_INT:         return "INT";
        case TOK_FLOAT:       return "FLOAT";
        case TOK_CHAR:        return "CHAR";
        case TOK_ID:          return "ID";
        case TOK_INT_LITERAL: return "INT_LITERAL";
        case TOK_ASSIGN:      return "ASSIGN";
        case TOK_LBRACKET:    return "LBRACKET";
        case TOK_RBRACKET:    return "RBRACKET";
        case TOK_LBRACE:      return "LBRACE";
        case TOK_RBRACE:      return "RBRACE";
        case TOK_SEMICOLON:   return "SEMICOLON";
        case TOK_LPAREN:      return "LPAREN";
        case TOK_RPAREN:      return "RPAREN";
        case TOK_COMMA:       return "COMMA";
        case TOK_PLUS:        return "PLUS";
        case TOK_MINUS:       return "MINUS";
        case TOK_MULTIPLY:    return "MULTIPLY";
        case TOK_DIVIDE:      return "DIVIDE";
        case TOK_EOF:         return "EOF";
        default:              return "INVALID";
    }
}

/* ── internal: keyword check ─────────────────────────────────── */
static TokenType checkKeyword(const char *word) {
    if (strcmp(word, "int")   == 0) return TOK_INT;
    if (strcmp(word, "float") == 0) return TOK_FLOAT;
    if (strcmp(word, "char")  == 0) return TOK_CHAR;
    return TOK_ID;
}

/* ── internal: add one token to the output array ─────────────── */
static int addToken(Token *tokens, int count, int maxTokens,
                    TokenType type, const char *lexeme,
                    int line, int col) {
    if (count >= maxTokens) return count;
    tokens[count].type   = type;
    tokens[count].line   = line;
    tokens[count].column = col;
    strncpy(tokens[count].lexeme, lexeme, MAX_LEXEME - 1);
    tokens[count].lexeme[MAX_LEXEME - 1] = '\0';
    return count + 1;
}

/* ── tokenize ─────────────────────────────────────────────────── */
int tokenize(const char *source, Token *tokens, int maxTokens) {
    int i      = 0;
    int line   = 1;
    int column = 1;
    int count  = 0;

    while (source[i] != '\0') {
        char ch = source[i];

        /* Skip whitespace */
        if (isspace((unsigned char)ch)) {
            if (ch == '\n') { line++; column = 1; }
            else            { column++; }
            i++;
            continue;
        }

        int startLine   = line;
        int startColumn = column;

        /* Identifiers and keywords */
        if (isalpha((unsigned char)ch) || ch == '_') {
            char word[MAX_LEXEME];
            int  j = 0;

            while (isalnum((unsigned char)source[i]) || source[i] == '_') {
                if (j < MAX_LEXEME - 1) word[j++] = source[i];
                i++; column++;
            }
            word[j] = '\0';

            TokenType type = checkKeyword(word);
            count = addToken(tokens, count, maxTokens,
                             type, word, startLine, startColumn);
            continue;
        }

        /* Integer literals */
        if (isdigit((unsigned char)ch)) {
            char number[MAX_LEXEME];
            int  j = 0;

            while (isdigit((unsigned char)source[i])) {
                if (j < MAX_LEXEME - 1) number[j++] = source[i];
                i++; column++;
            }
            number[j] = '\0';

            /* Decimal literals not supported */
            if (source[i] == '.' && isdigit((unsigned char)source[i + 1])) {
                fprintf(stderr,
                    "Lexical Error: decimal literal not supported "
                    "at line %d, column %d\n", line, column);
                count = addToken(tokens, count, maxTokens,
                                 TOK_INVALID, number, startLine, startColumn);
                return count;
            }

            count = addToken(tokens, count, maxTokens,
                             TOK_INT_LITERAL, number, startLine, startColumn);
            continue;
        }

        /* Single-character operators and delimiters */
        TokenType type;
        char symbol[2] = {ch, '\0'};

        switch (ch) {
            case '=': type = TOK_ASSIGN;    break;
            case '[': type = TOK_LBRACKET;  break;
            case ']': type = TOK_RBRACKET;  break;
            case '{': type = TOK_LBRACE;    break;
            case '}': type = TOK_RBRACE;    break;
            case ';': type = TOK_SEMICOLON; break;
            case '(': type = TOK_LPAREN;    break;
            case ')': type = TOK_RPAREN;    break;
            case ',': type = TOK_COMMA;     break;
            case '+': type = TOK_PLUS;      break;
            case '-': type = TOK_MINUS;     break;
            case '*': type = TOK_MULTIPLY;  break;
            case '/': type = TOK_DIVIDE;    break;

            default:
                fprintf(stderr,
                    "Lexical Error: invalid character '%c' "
                    "at line %d, column %d\n", ch, line, column);
                count = addToken(tokens, count, maxTokens,
                                 TOK_INVALID, symbol, startLine, startColumn);
                return count;
        }

        count = addToken(tokens, count, maxTokens,
                         type, symbol, startLine, startColumn);
        i++; column++;
    }

    /* Always end with EOF */
    count = addToken(tokens, count, maxTokens, TOK_EOF, "EOF", line, column);
    return count;
}
