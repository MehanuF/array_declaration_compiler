#ifndef MINI_COMPILER_PARSER_H
#define MINI_COMPILER_PARSER_H

#include <stdbool.h>

#include "ast.h"
#include "compiler.h"
#include "token.h"

bool parser_parse(const TokenArray *tokens, Program *program, CompilerError *error);

#endif
