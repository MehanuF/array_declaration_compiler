#ifndef MINI_COMPILER_LEXER_H
#define MINI_COMPILER_LEXER_H

#include <stdbool.h>

#include "compiler.h"
#include "token.h"

bool lexer_tokenize(const char *source, TokenArray *tokens, CompilerError *error);

#endif
