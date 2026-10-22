#ifndef MINI_COMPILER_AST_H
#define MINI_COMPILER_AST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef enum {
    EXPR_INTEGER,
    EXPR_FLOAT,
    EXPR_CHARACTER,
    EXPR_IDENTIFIER,
    EXPR_ARRAY_ACCESS,
    EXPR_UNARY,
    EXPR_BINARY
} ExpressionKind;

typedef struct Expression Expression;

struct Expression {
    ExpressionKind kind;
    int line;
    int column;
    union {
        long integer_value;
        double float_value;
        char character_value;
        char *identifier;
        struct {
            char *array_name;
            Expression *index;
        } array_access;
        struct {
            char operator;
            Expression *operand;
        } unary;
        struct {
            Expression *left;
            char operator;
            Expression *right;
        } binary;
    } as;
};

typedef enum {
    STATEMENT_ARRAY_DECLARATION,
    STATEMENT_ELEMENT_ASSIGNMENT,
    STATEMENT_SCALAR_ASSIGNMENT
} StatementKind;

typedef struct {
    StatementKind kind;
    int line;
    int column;
    union {
        struct {
            char *element_type;
            char *name;
            Expression *size;
            bool has_initializer;
            Expression **initializer;
            size_t initializer_count;
        } declaration;
        struct {
            char *array_name;
            Expression *index;
            Expression *value;
        } assignment;
        struct {
            char *name;
            Expression *value;
        } scalar_assignment;
    } as;
} Statement;

typedef struct {
    Statement **statements;
    size_t count;
    size_t capacity;
} Program;

void expression_free(Expression *expression);
void statement_free(Statement *statement);
void program_free(Program *program);
void ast_print_json(const Program *program, FILE *output);

#endif
