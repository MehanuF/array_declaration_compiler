#include "lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static void token_array_reset(TokenArray *tokens) {
    tokens->items = NULL;
    tokens->count = 0;
    tokens->capacity = 0;
}

static bool token_array_push(TokenArray *tokens, TokenType type,
                             const char *source, size_t start, size_t length,
                             int line, int column, CompilerError *error) {
    Token token;
    if (tokens->count == tokens->capacity) {
        size_t new_capacity = tokens->capacity == 0 ? 32 : tokens->capacity * 2;
        Token *new_items = realloc(tokens->items, new_capacity * sizeof(Token));
        if (new_items == NULL) {
            compiler_error_set(error, line, column, "out of memory");
            return false;
        }
        tokens->items = new_items;
        tokens->capacity = new_capacity;
    }

    token.type = type;
    token.lexeme = compiler_string_slice(source, start, length);
    if (token.lexeme == NULL) {
        compiler_error_set(error, line, column, "out of memory");
        return false;
    }
    token.line = line;
    token.column = column;

    tokens->items[tokens->count++] = token;
    return true;
}

static bool is_identifier_start(char ch) {
    return isalpha((unsigned char)ch) || ch == '_';
}

static bool is_identifier_part(char ch) {
    return isalnum((unsigned char)ch) || ch == '_';
}

static TokenType keyword_or_identifier(const char *text, size_t start, size_t length) {
    if (length == 3 && strncmp(text + start, "int", 3) == 0) {
        return TOKEN_TYPE;
    }
    if (length == 5 && strncmp(text + start, "float", 5) == 0) {
        return TOKEN_TYPE;
    }
    if (length == 4 && strncmp(text + start, "char", 4) == 0) {
        return TOKEN_TYPE;
    }
    return TOKEN_IDENTIFIER;
}

static bool lex_character_literal(const char *source, size_t *index,
                                  int *line, int *column,
                                  TokenArray *tokens, CompilerError *error) {
    size_t start = *index;
    int token_line = *line;
    int token_column = *column;

    (*index)++;
    (*column)++;

    if (source[*index] == '\0' || source[*index] == '\n') {
        compiler_error_set(error, token_line, token_column,
                           "unterminated character literal");
        return false;
    }

    if (source[*index] == '\\') {
        (*index)++;
        (*column)++;
        if (source[*index] == '\0' || source[*index] == '\n') {
            compiler_error_set(error, token_line, token_column,
                               "unterminated character literal");
            return false;
        }
    }

    (*index)++;
    (*column)++;

    if (source[*index] != '\'') {
        compiler_error_set(error, token_line, token_column,
                           "malformed character literal");
        return false;
    }

    (*index)++;
    (*column)++;

    return token_array_push(tokens, TOKEN_CHARACTER, source, start,
                            *index - start, token_line, token_column, error);
}

static bool skip_block_comment(const char *source, size_t *index,
                               int *line, int *column, CompilerError *error) {
    int start_line = *line;
    int start_column = *column;

    *index += 2;
    *column += 2;

    while (source[*index] != '\0') {
        if (source[*index] == '*' && source[*index + 1] == '/') {
            *index += 2;
            *column += 2;
            return true;
        }
        if (source[*index] == '\n') {
            (*line)++;
            *column = 1;
            (*index)++;
            continue;
        }
        (*index)++;
        (*column)++;
    }

    compiler_error_set(error, start_line, start_column,
                       "unterminated block comment");
    return false;
}

bool lexer_tokenize(const char *source, TokenArray *tokens, CompilerError *error) {
    size_t index = 0;
    int line = 1;
    int column = 1;

    compiler_error_clear(error);
    token_array_reset(tokens);

    while (source[index] != '\0') {
        char ch = source[index];
        size_t start = index;
        int token_line = line;
        int token_column = column;

        if (isspace((unsigned char)ch)) {
            if (ch == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            index++;
            continue;
        }

        if (ch == '/' && source[index + 1] == '/') {
            index += 2;
            column += 2;
            while (source[index] != '\0' && source[index] != '\n') {
                index++;
                column++;
            }
            continue;
        }

        if (ch == '/' && source[index + 1] == '*') {
            if (!skip_block_comment(source, &index, &line, &column, error)) {
                token_array_free(tokens);
                return false;
            }
            continue;
        }

        if (is_identifier_start(ch)) {
            index++;
            column++;
            while (is_identifier_part(source[index])) {
                index++;
                column++;
            }
            if (!token_array_push(tokens,
                                  keyword_or_identifier(source, start, index - start),
                                  source, start, index - start,
                                  token_line, token_column, error)) {
                token_array_free(tokens);
                return false;
            }
            continue;
        }

        if (isdigit((unsigned char)ch)) {
            bool is_float = false;
            index++;
            column++;
            while (isdigit((unsigned char)source[index])) {
                index++;
                column++;
            }
            if (source[index] == '.' && isdigit((unsigned char)source[index + 1])) {
                is_float = true;
                index++;
                column++;
                while (isdigit((unsigned char)source[index])) {
                    index++;
                    column++;
                }
            }
            if (!token_array_push(tokens,
                                  is_float ? TOKEN_FLOAT : TOKEN_INTEGER,
                                  source, start, index - start,
                                  token_line, token_column, error)) {
                token_array_free(tokens);
                return false;
            }
            continue;
        }

        if (ch == '\'') {
            if (!lex_character_literal(source, &index, &line, &column, tokens, error)) {
                token_array_free(tokens);
                return false;
            }
            continue;
        }

        switch (ch) {
            case '[':
                index++;
                column++;
                if (!token_array_push(tokens, TOKEN_LBRACKET, source, start, 1,
                                      token_line, token_column, error)) {
                    token_array_free(tokens);
                    return false;
                }
                break;
            case ']':
                index++;
                column++;
                if (!token_array_push(tokens, TOKEN_RBRACKET, source, start, 1,
                                      token_line, token_column, error)) {
                    token_array_free(tokens);
                    return false;
                }
                break;
            case '{':
                index++;
                column++;
                if (!token_array_push(tokens, TOKEN_LBRACE, source, start, 1,
                                      token_line, token_column, error)) {
                    token_array_free(tokens);
                    return false;
                }
                break;
            case '}':
                index++;
                column++;
                if (!token_array_push(tokens, TOKEN_RBRACE, source, start, 1,
                                      token_line, token_column, error)) {
                    token_array_free(tokens);
                    return false;
                }
                break;
            case '(':
                index++;
                column++;
                if (!token_array_push(tokens, TOKEN_LPAREN, source, start, 1,
                                      token_line, token_column, error)) {
                    token_array_free(tokens);
                    return false;
                }
                break;
            case ')':
                index++;
                column++;
                if (!token_array_push(tokens, TOKEN_RPAREN, source, start, 1,
                                      token_line, token_column, error)) {
                    token_array_free(tokens);
                    return false;
                }
                break;
            case ',':
                index++;
                column++;
                if (!token_array_push(tokens, TOKEN_COMMA, source, start, 1,
                                      token_line, token_column, error)) {
                    token_array_free(tokens);
                    return false;
                }
                break;
            case ';':
                index++;
                column++;
                if (!token_array_push(tokens, TOKEN_SEMICOLON, source, start, 1,
                                      token_line, token_column, error)) {
                    token_array_free(tokens);
                    return false;
                }
                break;
            case '=':
                index++;
                column++;
                if (!token_array_push(tokens, TOKEN_ASSIGN, source, start, 1,
                                      token_line, token_column, error)) {
                    token_array_free(tokens);
                    return false;
                }
                break;
            case '+':
                index++;
                column++;
                if (!token_array_push(tokens, TOKEN_PLUS, source, start, 1,
                                      token_line, token_column, error)) {
                    token_array_free(tokens);
                    return false;
                }
                break;
            case '-':
                index++;
                column++;
                if (!token_array_push(tokens, TOKEN_MINUS, source, start, 1,
                                      token_line, token_column, error)) {
                    token_array_free(tokens);
                    return false;
                }
                break;
            case '*':
                index++;
                column++;
                if (!token_array_push(tokens, TOKEN_STAR, source, start, 1,
                                      token_line, token_column, error)) {
                    token_array_free(tokens);
                    return false;
                }
                break;
            case '/':
                index++;
                column++;
                if (!token_array_push(tokens, TOKEN_SLASH, source, start, 1,
                                      token_line, token_column, error)) {
                    token_array_free(tokens);
                    return false;
                }
                break;
            case '%':
                index++;
                column++;
                if (!token_array_push(tokens, TOKEN_PERCENT, source, start, 1,
                                      token_line, token_column, error)) {
                    token_array_free(tokens);
                    return false;
                }
                break;
            default:
                compiler_error_set(error, token_line, token_column,
                                   "unexpected character '%c'", ch);
                token_array_free(tokens);
                return false;
        }
    }

    return token_array_push(tokens, TOKEN_EOF, source, index, 0, line, column, error);
}
