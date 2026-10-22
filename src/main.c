#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"

static bool read_file(const char *path, char **out_source) {
    FILE *file;
    long size;
    size_t bytes_read;
    char *buffer;

    file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }

    size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }

    buffer = malloc((size_t)size + 1);
    if (buffer == NULL) {
        fclose(file);
        return false;
    }

    bytes_read = fread(buffer, 1, (size_t)size, file);
    fclose(file);

    if (bytes_read != (size_t)size) {
        free(buffer);
        return false;
    }

    buffer[size] = '\0';
    *out_source = buffer;
    return true;
}

int main(int argc, char **argv) {
    char *source = NULL;
    TokenArray tokens;
    Program program;
    CompilerError error;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source-file>\n", argv[0]);
        return 2;
    }

    if (!read_file(argv[1], &source)) {
        fprintf(stderr, "Error: failed to read '%s'\n", argv[1]);
        return 2;
    }

    if (!lexer_tokenize(source, &tokens, &error)) {
        fprintf(stderr, "Lexical error at %d:%d: %s\n", error.line, error.column, error.message);
        free(source);
        return 1;
    }

    if (!parser_parse(&tokens, &program, &error)) {
        fprintf(stderr, "Syntax error at %d:%d: %s\n", error.line, error.column, error.message);
        token_array_free(&tokens);
        free(source);
        return 1;
    }

    if (!semantic_analyze(&program, &error)) {
        fprintf(stderr, "Semantic error at %d:%d: %s\n", error.line, error.column, error.message);
        program_free(&program);
        token_array_free(&tokens);
        free(source);
        return 1;
    }

    puts("ACCEPTED");

    program_free(&program);
    token_array_free(&tokens);
    free(source);
    return 0;
}
