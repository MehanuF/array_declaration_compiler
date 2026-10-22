#include "token.h"

#include <stdlib.h>

const char *token_type_name(TokenType type) {
    static const char *names[] = {
        "TYPE",       "IDENTIFIER", "INTEGER",    "FLOAT",      "CHARACTER",
        "LBRACKET",   "RBRACKET",  "LBRACE",     "RBRACE",     "LPAREN",
        "RPAREN",     "COMMA",      "SEMICOLON",  "ASSIGN",     "PLUS",
        "MINUS",      "STAR",       "SLASH",      "PERCENT",    "EOF"
    };
    return names[type];
}

void token_array_free(TokenArray *tokens) {
    size_t i;
    if (tokens == NULL) {
        return;
    }
    for (i = 0; i < tokens->count; ++i) {
        free(tokens->items[i].lexeme);
    }
    free(tokens->items);
    tokens->items = NULL;
    tokens->count = 0;
    tokens->capacity = 0;
}
