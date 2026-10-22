#ifndef MINI_COMPILER_SEMANTIC_H
#define MINI_COMPILER_SEMANTIC_H

#include <stdbool.h>

#include "ast.h"
#include "compiler.h"

bool semantic_analyze(const Program *program, CompilerError *error);

#endif
