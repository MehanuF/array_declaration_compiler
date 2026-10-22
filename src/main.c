#include "ast.h"
#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program) {
    fprintf(stderr,
            "Usage: %s [--tokens] [--ast] <source-file>\n"
            "       %s [--tokens] [--ast] -c \"source code\"\n",
            program, program);
}

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    long length;
    char *source;
    size_t read_count;
    if (file == NULL) {
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    source = malloc((size_t)length + 1);
    if (source == NULL) {
        fclose(file);
        return NULL;
    }
    read_count = fread(source, 1, (size_t)length, file);
    fclose(file);
    if (read_count != (size_t)length) {
        free(source);
        return NULL;
    }
    source[length] = '\0';
    return source;
}

static void print_error(const char *kind, const CompilerError *error,
                        const char *source) {
    const char *line_start = source;
    const char *line_end;
    int line = 1;
    int i;
    while (*line_start != '\0' && line < error->line) {
        if (*line_start++ == '\n') {
            line++;
        }
    }
    line_end = line_start;
    while (*line_end != '\0' && *line_end != '\n' && *line_end != '\r') {
        line_end++;
    }
    fprintf(stderr, "%s at line %d, column %d: %s\n", kind, error->line,
            error->column, error->message);
    if (line == error->line) {
        fprintf(stderr, "  %.*s\n  ", (int)(line_end - line_start), line_start);
        for (i = 1; i < error->column; ++i) {
            fputc(' ', stderr);
        }
        fputs("^\n", stderr);
    }
}

static void print_tokens(const TokenArray *tokens) {
    size_t i;
    puts("TOKENS");
    for (i = 0; i < tokens->count; ++i) {
        const Token *token = &tokens->items[i];
        if (token->type != TOKEN_EOF) {
            printf("%d:%d %-12s '%s'\n", token->line, token->column,
                   token_type_name(token->type), token->lexeme);
        }
    }
}

int main(int argc, char **argv) {
    bool show_tokens = false;
    bool show_ast = false;
    const char *path = NULL;
    const char *inline_code = NULL;
    char *owned_source = NULL;
    const char *source;
    TokenArray tokens;
    Program program;
    CompilerError error;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--tokens") == 0) {
            show_tokens = true;
        } else if (strcmp(argv[i], "--ast") == 0) {
            show_ast = true;
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            inline_code = argv[++i];
        } else if (path == NULL) {
            path = argv[i];
        } else {
            print_usage(argv[0]);
            return 2;
        }
    }
    if ((path == NULL) == (inline_code == NULL)) {
        print_usage(argv[0]);
        return 2;
    }
    if (!show_tokens && !show_ast) {
        show_tokens = true;
        show_ast = true;
    }

    if (inline_code != NULL) {
        source = inline_code;
    } else {
        owned_source = read_file(path);
        if (owned_source == NULL) {
            fprintf(stderr, "could not read source file '%s'\n", path);
            return 1;
        }
        source = owned_source;
    }

    if (!lexer_tokenize(source, &tokens, &error)) {
        print_error("lexical error", &error, source);
        free(owned_source);
        return 1;
    }
    if (show_tokens) {
        print_tokens(&tokens);
    }
    if (!parser_parse(&tokens, &program, &error)) {
        print_error("syntax error", &error, source);
        token_array_free(&tokens);
        free(owned_source);
        return 1;
    }
    if (show_tokens && show_ast) {
        putchar('\n');
    }
    if (show_ast) {
        puts("AST");
        ast_print_json(&program, stdout);
    }

    program_free(&program);
    token_array_free(&tokens);
    free(owned_source);
    return 0;
}
