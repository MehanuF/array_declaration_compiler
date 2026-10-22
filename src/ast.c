#include "ast.h"

#include <stdlib.h>

void expression_free(Expression *expression) {
    if (expression == NULL) {
        return;
    }
    switch (expression->kind) {
        case EXPR_IDENTIFIER:
            free(expression->as.identifier);
            break;
        case EXPR_ARRAY_ACCESS:
            free(expression->as.array_access.array_name);
            expression_free(expression->as.array_access.index);
            break;
        case EXPR_UNARY:
            expression_free(expression->as.unary.operand);
            break;
        case EXPR_BINARY:
            expression_free(expression->as.binary.left);
            expression_free(expression->as.binary.right);
            break;
        default:
            break;
    }
    free(expression);
}

void statement_free(Statement *statement) {
    size_t i;
    if (statement == NULL) {
        return;
    }
    if (statement->kind == STATEMENT_ARRAY_DECLARATION) {
        free(statement->as.declaration.element_type);
        free(statement->as.declaration.name);
        expression_free(statement->as.declaration.size);
        for (i = 0; i < statement->as.declaration.initializer_count; ++i) {
            expression_free(statement->as.declaration.initializer[i]);
        }
        free(statement->as.declaration.initializer);
    } else if (statement->kind == STATEMENT_ELEMENT_ASSIGNMENT) {
        free(statement->as.assignment.array_name);
        expression_free(statement->as.assignment.index);
        expression_free(statement->as.assignment.value);
    } else {
        free(statement->as.scalar_assignment.name);
        expression_free(statement->as.scalar_assignment.value);
    }
    free(statement);
}

void program_free(Program *program) {
    size_t i;
    if (program == NULL) {
        return;
    }
    for (i = 0; i < program->count; ++i) {
        statement_free(program->statements[i]);
    }
    free(program->statements);
    program->statements = NULL;
    program->count = 0;
    program->capacity = 0;
}

static void indent(FILE *output, int depth) {
    int i;
    for (i = 0; i < depth; ++i) {
        fputs("  ", output);
    }
}

static void print_json_string(FILE *output, const char *text) {
    const unsigned char *cursor = (const unsigned char *)text;
    fputc('"', output);
    while (*cursor != '\0') {
        switch (*cursor) {
            case '"': fputs("\\\"", output); break;
            case '\\': fputs("\\\\", output); break;
            case '\n': fputs("\\n", output); break;
            case '\r': fputs("\\r", output); break;
            case '\t': fputs("\\t", output); break;
            case '\0': fputs("\\u0000", output); break;
            default:
                if (*cursor < 32) {
                    fprintf(output, "\\u%04x", *cursor);
                } else {
                    fputc(*cursor, output);
                }
        }
        cursor++;
    }
    fputc('"', output);
}

static void print_location(FILE *output, const Expression *expression) {
    fprintf(output, ", \"line\": %d, \"column\": %d", expression->line, expression->column);
}

static void print_expression(const Expression *expression, FILE *output, int depth) {
    if (expression == NULL) {
        fputs("null", output);
        return;
    }
    fputs("{\n", output);
    indent(output, depth + 1);
    switch (expression->kind) {
        case EXPR_INTEGER:
            fprintf(output, "\"node\": \"IntegerLiteral\", \"value\": %ld", expression->as.integer_value);
            break;
        case EXPR_FLOAT:
            fprintf(output, "\"node\": \"FloatLiteral\", \"value\": %.15g", expression->as.float_value);
            break;
        case EXPR_CHARACTER:
            fputs("\"node\": \"CharacterLiteral\", \"value\": ", output);
            if (expression->as.character_value == '\0') {
                fputs("\"\\u0000\"", output);
            } else {
                char value[2] = {expression->as.character_value, '\0'};
                print_json_string(output, value);
            }
            break;
        case EXPR_IDENTIFIER:
            fputs("\"node\": \"Identifier\", \"name\": ", output);
            print_json_string(output, expression->as.identifier);
            break;
        case EXPR_ARRAY_ACCESS:
            fputs("\"node\": \"ArrayAccess\", \"array\": ", output);
            print_json_string(output, expression->as.array_access.array_name);
            print_location(output, expression);
            fputs(",\n", output);
            indent(output, depth + 1);
            fputs("\"index\": ", output);
            print_expression(expression->as.array_access.index, output, depth + 1);
            fputc('\n', output);
            indent(output, depth);
            fputc('}', output);
            return;
        case EXPR_UNARY:
            fprintf(output, "\"node\": \"UnaryExpression\", \"operator\": \"%c\"",
                    expression->as.unary.operator);
            print_location(output, expression);
            fputs(",\n", output);
            indent(output, depth + 1);
            fputs("\"operand\": ", output);
            print_expression(expression->as.unary.operand, output, depth + 1);
            fputc('\n', output);
            indent(output, depth);
            fputc('}', output);
            return;
        case EXPR_BINARY:
            fprintf(output, "\"node\": \"BinaryExpression\", \"operator\": \"%c\"",
                    expression->as.binary.operator);
            print_location(output, expression);
            fputs(",\n", output);
            indent(output, depth + 1);
            fputs("\"left\": ", output);
            print_expression(expression->as.binary.left, output, depth + 1);
            fputs(",\n", output);
            indent(output, depth + 1);
            fputs("\"right\": ", output);
            print_expression(expression->as.binary.right, output, depth + 1);
            fputc('\n', output);
            indent(output, depth);
            fputc('}', output);
            return;
    }
    print_location(output, expression);
    fputc('\n', output);
    indent(output, depth);
    fputc('}', output);
}

static void print_statement(const Statement *statement, FILE *output, int depth) {
    size_t i;
    fputs("{\n", output);
    indent(output, depth + 1);
    if (statement->kind == STATEMENT_ARRAY_DECLARATION) {
        fputs("\"node\": \"ArrayDeclaration\", \"type\": ", output);
        print_json_string(output, statement->as.declaration.element_type);
        fputs(", \"name\": ", output);
        print_json_string(output, statement->as.declaration.name);
        fprintf(output, ", \"line\": %d, \"column\": %d,\n", statement->line, statement->column);
        indent(output, depth + 1);
        fputs("\"size\": ", output);
        print_expression(statement->as.declaration.size, output, depth + 1);
        fputs(",\n", output);
        indent(output, depth + 1);
        fputs("\"initializer\": ", output);
        if (!statement->as.declaration.has_initializer) {
            fputs("null\n", output);
        } else {
            fputs("[", output);
            for (i = 0; i < statement->as.declaration.initializer_count; ++i) {
                if (i > 0) {
                    fputc(',', output);
                }
                fputc('\n', output);
                indent(output, depth + 2);
                print_expression(statement->as.declaration.initializer[i], output, depth + 2);
            }
            if (statement->as.declaration.initializer_count > 0) {
                fputc('\n', output);
                indent(output, depth + 1);
            }
            fputs("]\n", output);
        }
    } else if (statement->kind == STATEMENT_ELEMENT_ASSIGNMENT) {
        fputs("\"node\": \"ElementAssignment\", \"array\": ", output);
        print_json_string(output, statement->as.assignment.array_name);
        fprintf(output, ", \"line\": %d, \"column\": %d,\n", statement->line, statement->column);
        indent(output, depth + 1);
        fputs("\"index\": ", output);
        print_expression(statement->as.assignment.index, output, depth + 1);
        fputs(",\n", output);
        indent(output, depth + 1);
        fputs("\"value\": ", output);
        print_expression(statement->as.assignment.value, output, depth + 1);
        fputc('\n', output);
    } else {
        fputs("\"node\": \"ScalarAssignment\", \"target\": ", output);
        print_json_string(output, statement->as.scalar_assignment.name);
        fprintf(output, ", \"line\": %d, \"column\": %d,\n", statement->line, statement->column);
        indent(output, depth + 1);
        fputs("\"value\": ", output);
        print_expression(statement->as.scalar_assignment.value, output, depth + 1);
        fputc('\n', output);
    }
    indent(output, depth);
    fputc('}', output);
}

void ast_print_json(const Program *program, FILE *output) {
    size_t i;
    fputs("{\n  \"node\": \"Program\",\n  \"statements\": [", output);
    for (i = 0; i < program->count; ++i) {
        if (i > 0) {
            fputc(',', output);
        }
        fputs("\n    ", output);
        print_statement(program->statements[i], output, 2);
    }
    if (program->count > 0) {
        fputc('\n', output);
    }
    fputs("  ]\n}\n", output);
}
