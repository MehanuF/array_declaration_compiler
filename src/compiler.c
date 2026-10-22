#include "compiler.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void compiler_error_clear(CompilerError *error) {
    if (error == NULL) {
        return;
    }
    error->occurred = false;
    error->line = 0;
    error->column = 0;
    error->message[0] = '\0';
}

void compiler_error_set(CompilerError *error, int line, int column,
                        const char *format, ...) {
    va_list arguments;

    if (error == NULL || error->occurred) {
        return;
    }
    error->occurred = true;
    error->line = line;
    error->column = column;
    va_start(arguments, format);
    vsnprintf(error->message, sizeof(error->message), format, arguments);
    va_end(arguments);
}

char *compiler_string_duplicate(const char *text) {
    return compiler_string_slice(text, 0, strlen(text));
}

char *compiler_string_slice(const char *text, size_t start, size_t length) {
    char *copy = malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, text + start, length);
    copy[length] = '\0';
    return copy;
}
