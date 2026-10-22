#include "ast.h"
#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define ASSERT(condition) do {                                                \
    if (!(condition)) {                                                       \
        fprintf(stderr, "assertion failed at %s:%d: %s\n",                  \
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

static bool test_lexer_declaration(void) {
    TokenArray tokens;
    CompilerError error;
    const TokenType expected[] = {
        TOKEN_TYPE, TOKEN_IDENTIFIER, TOKEN_LBRACKET, TOKEN_INTEGER,
        TOKEN_RBRACKET, TOKEN_SEMICOLON, TOKEN_EOF
    };
    size_t i;
    ASSERT(lexer_tokenize("int arr[5];", &tokens, &error));
    ASSERT(tokens.count == sizeof(expected) / sizeof(expected[0]));
    for (i = 0; i < tokens.count; ++i) {
        ASSERT(tokens.items[i].type == expected[i]);
    }
    ASSERT(strcmp(tokens.items[1].lexeme, "arr") == 0);
    ASSERT(strcmp(tokens.items[3].lexeme, "5") == 0);
    token_array_free(&tokens);
    return true;
}

static bool test_lexer_comments_and_locations(void) {
    TokenArray tokens;
    CompilerError error;
    ASSERT(lexer_tokenize("// heading\nint /* size */ values[3];", &tokens, &error));
    ASSERT(tokens.items[0].line == 2);
    ASSERT(tokens.items[1].column == 16);
    token_array_free(&tokens);
    return true;
}

static bool test_lexer_literals(void) {
    TokenArray tokens;
    CompilerError error;
    size_t i;
    bool saw_float = false;
    bool saw_character = false;
    ASSERT(lexer_tokenize("float a[] = {1.5}; char c[] = {'\\n'};", &tokens, &error));
    for (i = 0; i < tokens.count; ++i) {
        saw_float = saw_float || tokens.items[i].type == TOKEN_FLOAT;
        saw_character = saw_character || tokens.items[i].type == TOKEN_CHARACTER;
    }
    ASSERT(saw_float && saw_character);
    token_array_free(&tokens);
    return true;
}

static bool test_lexer_invalid_character(void) {
    TokenArray tokens;
    CompilerError error;
    ASSERT(!lexer_tokenize("int a[2];\na[0] = @;", &tokens, &error));
    ASSERT(error.occurred);
    ASSERT(error.line == 2 && error.column == 8);
    ASSERT(strstr(error.message, "unexpected character") != NULL);
    return true;
}

static bool test_lexer_unterminated_comment(void) {
    TokenArray tokens;
    CompilerError error;
    ASSERT(!lexer_tokenize("int a[2]; /* not closed", &tokens, &error));
    ASSERT(strstr(error.message, "unterminated block comment") != NULL);
    return true;
}

static bool test_parser_sized_declaration(void) {
    Program program;
    CompilerError error;
    Statement *statement;
    ASSERT(parse_source("int arr[5];", &program, &error));
    ASSERT(program.count == 1);
    statement = program.statements[0];
    ASSERT(statement->kind == STATEMENT_ARRAY_DECLARATION);
    ASSERT(strcmp(statement->as.declaration.element_type, "int") == 0);
    ASSERT(strcmp(statement->as.declaration.name, "arr") == 0);
    ASSERT(statement->as.declaration.size->kind == EXPR_INTEGER);
    ASSERT(statement->as.declaration.size->as.integer_value == 5);
    ASSERT(!statement->as.declaration.has_initializer);
    program_free(&program);
    return true;
}

static bool test_parser_initializer(void) {
    Program program;
    CompilerError error;
    Statement *statement;
    ASSERT(parse_source("int data[] = {1, 2, 3,};", &program, &error));
    statement = program.statements[0];
    ASSERT(statement->as.declaration.size == NULL);
    ASSERT(statement->as.declaration.has_initializer);
    ASSERT(statement->as.declaration.initializer_count == 3);
    ASSERT(statement->as.declaration.initializer[2]->as.integer_value == 3);
    program_free(&program);
    return true;
}

static bool test_parser_assignment_and_precedence(void) {
    Program program;
    CompilerError error;
    Statement *statement;
    ASSERT(parse_source("arr[1 + 2] = other[0] * 4;", &program, &error));
    statement = program.statements[0];
    ASSERT(statement->kind == STATEMENT_ELEMENT_ASSIGNMENT);
    ASSERT(statement->as.assignment.index->kind == EXPR_BINARY);
    ASSERT(statement->as.assignment.index->as.binary.operator == '+');
    ASSERT(statement->as.assignment.value->kind == EXPR_BINARY);
    ASSERT(statement->as.assignment.value->as.binary.operator == '*');
    program_free(&program);
    return true;
}

static bool test_parser_multiple_statements(void) {
    Program program;
    CompilerError error;
    ASSERT(parse_source("int a[2];\na[0] = -7;", &program, &error));
    ASSERT(program.count == 2);
    ASSERT(program.statements[1]->as.assignment.value->kind == EXPR_UNARY);
    program_free(&program);
    return true;
}

static bool test_parser_scalar_assignment_from_member_grammar(void) {
    Program program;
    CompilerError error;
    Statement *statement;
    ASSERT(parse_source("x = arr[2];", &program, &error));
    ASSERT(program.count == 1);
    statement = program.statements[0];
    ASSERT(statement->kind == STATEMENT_SCALAR_ASSIGNMENT);
    ASSERT(strcmp(statement->as.scalar_assignment.name, "x") == 0);
    ASSERT(statement->as.scalar_assignment.value->kind == EXPR_ARRAY_ACCESS);
    ASSERT(strcmp(statement->as.scalar_assignment.value->as.array_access.array_name,
                  "arr") == 0);
    program_free(&program);
    return true;
}

static bool test_parser_missing_bracket(void) {
    Program program;
    CompilerError error;
    ASSERT(!parse_source("int arr[5;", &program, &error));
    ASSERT(strstr(error.message, "expected ']'") != NULL);
    return true;
}

static bool test_parser_missing_semicolon(void) {
    Program program;
    CompilerError error;
    ASSERT(!parse_source("int arr[5]", &program, &error));
    ASSERT(strstr(error.message, "expected ';'") != NULL);
    return true;
}

static bool test_parser_unsized_without_initializer(void) {
    Program program;
    CompilerError error;
    ASSERT(!parse_source("int arr[];", &program, &error));
    ASSERT(strstr(error.message, "requires an initializer") != NULL);
    return true;
}

static bool test_all_member_grammar_cases(void) {
    const char *valid[] = {
        "int arr[5];",
        "float values[10];",
        "char letters[4];",
        "int marks[3] = {10, 20, 30};",
        "int a[2] = {1, 2};",
        "arr[2] = 50;",
        "x = arr[2];",
        "arr[1 + 2] = 8;",
        "int a[4] = {1 + 2, 5, 8, 10};",
        "int arr[3] = {1, 2, 3};"
    };
    const char *invalid[] = {
        "int [5];",
        "int arr[5]",
        "int arr[];",
        "int arr[5 = 2;",
        "int arr[3] = {1, 2, 3;",
        "arr[2] = ;",
        "int arr[3] = {, 1};"
    };
    size_t i;
    CompilerError error;
    Program program;

    for (i = 0; i < sizeof(valid) / sizeof(valid[0]); ++i) {
        ASSERT(parse_source(valid[i], &program, &error));
        program_free(&program);
    }
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        ASSERT(!parse_source(invalid[i], &program, &error));
        ASSERT(error.occurred);
    }
    return true;
}

typedef bool (*TestFunction)(void);

typedef struct {
    const char *name;
    TestFunction function;
} TestCase;

int main(void) {
    const TestCase tests[] = {
        {"lexer declaration", test_lexer_declaration},
        {"lexer comments and locations", test_lexer_comments_and_locations},
        {"lexer literals", test_lexer_literals},
        {"lexer invalid character", test_lexer_invalid_character},
        {"lexer unterminated comment", test_lexer_unterminated_comment},
        {"parser sized declaration", test_parser_sized_declaration},
        {"parser initializer", test_parser_initializer},
        {"parser assignment and precedence", test_parser_assignment_and_precedence},
        {"parser multiple statements", test_parser_multiple_statements},
        {"parser scalar assignment from member grammar", test_parser_scalar_assignment_from_member_grammar},
        {"parser missing bracket", test_parser_missing_bracket},
        {"parser missing semicolon", test_parser_missing_semicolon},
        {"parser unsized without initializer", test_parser_unsized_without_initializer},
        {"all member grammar cases", test_all_member_grammar_cases}
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
