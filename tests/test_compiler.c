#include "ast.h"
#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "token.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define ASSERT(condition) do {                                                \
    if (!(condition)) {                                                       \
        fprintf(stderr, "assertion failed at %s:%d: %s\n",                 \
                __FILE__, __LINE__, #condition);                              \
        return false;                                                         \
    }                                                                         \
} while (0)

static bool parse_source(const char *source, Program *program, CompilerError *error) {
    TokenArray tokens;
    bool result;

    if (!lexer_tokenize(source, &tokens, error)) {
        return false;
    }
    result = parser_parse(&tokens, program, error);
    token_array_free(&tokens);
    return result;
}

static bool analyze_source(const char *source, CompilerError *error) {
    TokenArray tokens;
    Program program;
    bool result;

    if (!lexer_tokenize(source, &tokens, error)) {
        return false;
    }
    if (!parser_parse(&tokens, &program, error)) {
        token_array_free(&tokens);
        return false;
    }
    result = semantic_analyze(&program, error);
    program_free(&program);
    token_array_free(&tokens);
    return result;
}

static bool test_lexer_literals_comments_locations(void) {
    TokenArray tokens;
    CompilerError error;
    bool saw_float = false;
    bool saw_char = false;
    size_t i;

    ASSERT(lexer_tokenize("// comment\nint a[2] = {1, 2,}; char c[1] = {'\\n'}; float f[] = {1.5};", &tokens, &error));
    ASSERT(tokens.items[0].line == 2);

    for (i = 0; i < tokens.count; ++i) {
        saw_float = saw_float || tokens.items[i].type == TOKEN_FLOAT;
        saw_char = saw_char || tokens.items[i].type == TOKEN_CHARACTER;
    }
    ASSERT(saw_float);
    ASSERT(saw_char);
    token_array_free(&tokens);
    return true;
}

static bool test_lexer_malformed_input(void) {
    TokenArray tokens;
    CompilerError error;

    ASSERT(!lexer_tokenize("int a[2]; a[0] = @;", &tokens, &error));
    ASSERT(strstr(error.message, "unexpected character") != NULL);

    ASSERT(!lexer_tokenize("int a[1]; /* open", &tokens, &error));
    ASSERT(strstr(error.message, "unterminated block comment") != NULL);
    return true;
}

static bool test_parser_precedence_unary_and_trailing_comma(void) {
    Program program;
    CompilerError error;
    Statement *statement;

    ASSERT(parse_source("int a[3] = {1,2,3,}; a[1] = -(2 + 3) * 4 % 3;", &program, &error));
    ASSERT(program.count == 2);

    statement = program.statements[1];
    ASSERT(statement->kind == STATEMENT_ELEMENT_ASSIGNMENT);
    ASSERT(statement->as.assignment.value->kind == EXPR_BINARY);
    ASSERT(statement->as.assignment.value->as.binary.operator == '%');
    ASSERT(statement->as.assignment.value->as.binary.left->kind == EXPR_BINARY);

    program_free(&program);
    return true;
}

static bool test_parser_rejects_malformed_cases(void) {
    Program program;
    CompilerError error;

    ASSERT(!parse_source("int arr[5;", &program, &error));
    ASSERT(strstr(error.message, "expected ']'") != NULL);

    ASSERT(!parse_source("int arr[5]", &program, &error));
    ASSERT(strstr(error.message, "expected ';'") != NULL);

    ASSERT(!parse_source("int arr[];", &program, &error));
    ASSERT(strstr(error.message, "requires an initializer") != NULL);
    return true;
}

static bool test_semantic_valid_program(void) {
    CompilerError error;
    ASSERT(analyze_source(
        "int a[3] = {1,2,3};"
        "float f[] = {1.0, 2, 'A'};"
        "char c[2] = {'A', 66};"
        "a[1] = +2;"
        "f[0] = a[1] / 2 + 3.5;",
        &error));
    return true;
}

static bool test_semantic_duplicate_declaration(void) {
    CompilerError error;
    ASSERT(!analyze_source("int a[2]; int a[3];", &error));
    ASSERT(strstr(error.message, "duplicate declaration") != NULL);
    return true;
}

static bool test_semantic_undeclared_identifier_and_array(void) {
    CompilerError error;
    ASSERT(!analyze_source("x = 1;", &error));
    ASSERT(strstr(error.message, "undeclared identifier") != NULL);

    ASSERT(!analyze_source("int a[2]; b[0] = 1;", &error));
    ASSERT(strstr(error.message, "undeclared array") != NULL);
    return true;
}

static bool test_semantic_index_rules(void) {
    CompilerError error;

    ASSERT(!analyze_source("int a[2]; a[2] = 1;", &error));
    ASSERT(strstr(error.message, "out of bounds") != NULL);

    ASSERT(!analyze_source("int a[2]; a[1.5] = 1;", &error));
    ASSERT(strstr(error.message, "must be integer") != NULL);
    return true;
}

static bool test_semantic_initializer_count_and_size(void) {
    CompilerError error;

    ASSERT(!analyze_source("int a[0];", &error));
    ASSERT(strstr(error.message, "must be positive") != NULL);

    ASSERT(!analyze_source("int a[2] = {1,2,3};", &error));
    ASSERT(strstr(error.message, "initializer count") != NULL);
    return true;
}

static bool test_semantic_type_mismatch(void) {
    CompilerError error;

    ASSERT(!analyze_source("int a[2]; a[0] = 1.5;", &error));
    ASSERT(strstr(error.message, "cannot assign") != NULL);

    ASSERT(!analyze_source("char c[1] = {1.5};", &error));
    ASSERT(strstr(error.message, "initializer type") != NULL);
    return true;
}

typedef bool (*TestFunction)(void);

typedef struct {
    const char *name;
    TestFunction function;
} TestCase;

int main(void) {
    const TestCase tests[] = {
        {"lexer literals/comments/locations", test_lexer_literals_comments_locations},
        {"lexer malformed input", test_lexer_malformed_input},
        {"parser precedence/unary/trailing comma", test_parser_precedence_unary_and_trailing_comma},
        {"parser rejects malformed cases", test_parser_rejects_malformed_cases},
        {"semantic valid program", test_semantic_valid_program},
        {"semantic duplicate declaration", test_semantic_duplicate_declaration},
        {"semantic undeclared identifier and array", test_semantic_undeclared_identifier_and_array},
        {"semantic index rules", test_semantic_index_rules},
        {"semantic initializer count and size", test_semantic_initializer_count_and_size},
        {"semantic type mismatch", test_semantic_type_mismatch}
    };
    const size_t count = sizeof(tests) / sizeof(tests[0]);
    size_t i;
    size_t passed = 0;

    for (i = 0; i < count; ++i) {
        if (tests[i].function()) {
            printf("PASS: %s\n", tests[i].name);
            passed++;
        } else {
            printf("FAIL: %s\n", tests[i].name);
        }
    }

    printf("\n%zu/%zu tests passed\n", passed, count);
    return passed == count ? 0 : 1;
}
