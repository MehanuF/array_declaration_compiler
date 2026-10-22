#include "lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *source;
    size_t current;
    int line;
    int column;
    TokenArray *tokens;
    CompilerError *error;
} Lexer;

static bool at_end(const Lexer *lexer) {
    return lexer->source[lexer->current] == '\0';
}

static char peek(const Lexer *lexer) {
    return lexer->source[lexer->current];
}

static char peek_next(const Lexer *lexer) {
    if (at_end(lexer)) {
        return '\0';
    }
    return lexer->source[lexer->current + 1];
}

static char advance(Lexer *lexer) {
    char character = lexer->source[lexer->current++];
    if (character == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return character;
}

static bool add_token(Lexer *lexer, TokenType type, size_t start,
                      int line, int column) {
    Token *larger;
    Token token;
    size_t length = lexer->current - start;

    if (lexer->tokens->count == lexer->tokens->capacity) {
        size_t capacity = lexer->tokens->capacity == 0 ? 16 : lexer->tokens->capacity * 2;
        larger = realloc(lexer->tokens->items, capacity * sizeof(*larger));
        if (larger == NULL) {
            compiler_error_set(lexer->error, line, column, "out of memory while storing tokens");
            return false;
        }
        lexer->tokens->items = larger;
        lexer->tokens->capacity = capacity;
    }

    token.type = type;
    token.lexeme = compiler_string_slice(lexer->source, start, length);
    token.line = line;
    token.column = column;
    if (token.lexeme == NULL) {
        compiler_error_set(lexer->error, line, column, "out of memory while storing a token");
        return false;
    }
    lexer->tokens->items[lexer->tokens->count++] = token;
    return true;
}

static TokenType single_character_type(char character) {
    switch (character) {
        case '[': return TOKEN_LBRACKET;
        case ']': return TOKEN_RBRACKET;
        case '{': return TOKEN_LBRACE;
        case '}': return TOKEN_RBRACE;
        case '(': return TOKEN_LPAREN;
        case ')': return TOKEN_RPAREN;
        case ',': return TOKEN_COMMA;
        case ';': return TOKEN_SEMICOLON;
        case '=': return TOKEN_ASSIGN;
        case '+': return TOKEN_PLUS;
        case '-': return TOKEN_MINUS;
        case '*': return TOKEN_STAR;
        case '/': return TOKEN_SLASH;
        case '%': return TOKEN_PERCENT;
        default: return TOKEN_EOF;
    }
}

static bool scan_identifier(Lexer *lexer, size_t start, int line, int column) {
    char *word;
    TokenType type;
    while (isalnum((unsigned char)peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }
    word = compiler_string_slice(lexer->source, start, lexer->current - start);
    if (word == NULL) {
        compiler_error_set(lexer->error, line, column, "out of memory while reading identifier");
        return false;
    }
    type = (strcmp(word, "int") == 0 || strcmp(word, "float") == 0 ||
            strcmp(word, "char") == 0) ? TOKEN_TYPE : TOKEN_IDENTIFIER;
    free(word);
    return add_token(lexer, type, start, line, column);
}

static bool scan_number(Lexer *lexer, size_t start, int line, int column) {
    TokenType type = TOKEN_INTEGER;
    while (isdigit((unsigned char)peek(lexer))) {
        advance(lexer);
    }
    if (peek(lexer) == '.' && isdigit((unsigned char)peek_next(lexer))) {
        type = TOKEN_FLOAT;
        advance(lexer);
        while (isdigit((unsigned char)peek(lexer))) {
            advance(lexer);
        }
    }
    return add_token(lexer, type, start, line, column);
}

static bool valid_escape(char character) {
    return character == 'n' || character == 't' || character == 'r' ||
           character == '0' || character == '\\' || character == '\'';
}

static bool scan_character(Lexer *lexer, size_t start, int line, int column) {
    advance(lexer); /* opening quote */
    if (at_end(lexer) || peek(lexer) == '\n') {
        compiler_error_set(lexer->error, line, column, "unterminated character literal");
        return false;
    }
    if (peek(lexer) == '\\') {
        advance(lexer);
        if (!valid_escape(peek(lexer))) {
            compiler_error_set(lexer->error, lexer->line, lexer->column,
                               "unknown escape sequence '\\%c'", peek(lexer));
            return false;
        }
        advance(lexer);
    } else {
        advance(lexer);
    }
    if (peek(lexer) != '\'') {
        compiler_error_set(lexer->error, line, column,
                           "character literal must contain exactly one character");
        return false;
    }
    advance(lexer);
    return add_token(lexer, TOKEN_CHARACTER, start, line, column);
}

static bool skip_block_comment(Lexer *lexer, int line, int column) {
    advance(lexer);
    advance(lexer);
    while (!at_end(lexer)) {
        if (peek(lexer) == '*' && peek_next(lexer) == '/') {
            advance(lexer);
            advance(lexer);
            return true;
        }
        advance(lexer);
    }
    compiler_error_set(lexer->error, line, column, "unterminated block comment");
    return false;
}

bool lexer_tokenize(const char *source, TokenArray *tokens, CompilerError *error) {
    Lexer lexer = {source, 0, 1, 1, tokens, error};
    compiler_error_clear(error);
    tokens->items = NULL;
    tokens->count = 0;
    tokens->capacity = 0;

    while (!at_end(&lexer)) {
        size_t start = lexer.current;
        int line = lexer.line;
        int column = lexer.column;
        char character = peek(&lexer);
        TokenType type;

        if (character == ' ' || character == '\t' || character == '\r' || character == '\n') {
            advance(&lexer);
            continue;
        }
        if (character == '/' && peek_next(&lexer) == '/') {
            while (!at_end(&lexer) && peek(&lexer) != '\n') {
                advance(&lexer);
            }
            continue;
        }
        if (character == '/' && peek_next(&lexer) == '*') {
            if (!skip_block_comment(&lexer, line, column)) {
                token_array_free(tokens);
                return false;
            }
            continue;
        }
        if (isalpha((unsigned char)character) || character == '_') {
            if (!scan_identifier(&lexer, start, line, column)) {
                token_array_free(tokens);
                return false;
            }
            continue;
        }
        if (isdigit((unsigned char)character)) {
            if (!scan_number(&lexer, start, line, column)) {
                token_array_free(tokens);
                return false;
            }
            continue;
        }
        if (character == '\'') {
            if (!scan_character(&lexer, start, line, column)) {
                token_array_free(tokens);
                return false;
            }
            continue;
        }

        type = single_character_type(character);
        if (type != TOKEN_EOF) {
            advance(&lexer);
            if (!add_token(&lexer, type, start, line, column)) {
                token_array_free(tokens);
                return false;
            }
            continue;
        }

        compiler_error_set(error, line, column, "unexpected character '%c'", character);
        token_array_free(tokens);
        return false;
    }

    if (!add_token(&lexer, TOKEN_EOF, lexer.current, lexer.line, lexer.column)) {
        token_array_free(tokens);
        return false;
    }
    return true;
}
