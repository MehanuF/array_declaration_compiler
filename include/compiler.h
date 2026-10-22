#ifndef MINI_COMPILER_COMPILER_H
#define MINI_COMPILER_COMPILER_H

#include <stdbool.h>
#include <stddef.h>

#define COMPILER_ERROR_MESSAGE_SIZE 256

typedef struct {
    bool occurred;
    int line;
    int column;
    char message[COMPILER_ERROR_MESSAGE_SIZE];
} CompilerError;

void compiler_error_clear(CompilerError *error);
void compiler_error_set(CompilerError *error, int line, int column,
                        const char *format, ...);
char *compiler_string_duplicate(const char *text);
char *compiler_string_slice(const char *text, size_t start, size_t length);

#endif
