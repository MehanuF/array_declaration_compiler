#include "ast.h"

#include <stdlib.h>
#include <string.h>

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

    switch (statement->kind) {
        case STATEMENT_ARRAY_DECLARATION:
            free(statement->as.declaration.element_type);
            free(statement->as.declaration.name);
            expression_free(statement->as.declaration.size);
            for (i = 0; i < statement->as.declaration.initializer_count; ++i) {
                expression_free(statement->as.declaration.initializer[i]);
            }
            free(statement->as.declaration.initializer);
            break;
        case STATEMENT_ELEMENT_ASSIGNMENT:
            free(statement->as.assignment.array_name);
            expression_free(statement->as.assignment.index);
            expression_free(statement->as.assignment.value);
            break;
        case STATEMENT_SCALAR_ASSIGNMENT:
            free(statement->as.scalar_assignment.name);
            expression_free(statement->as.scalar_assignment.value);
            break;
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

void ast_print_json(const Program *program, FILE *output) {
    size_t i;
    if (program == NULL || output == NULL) {
        return;
    }

    fprintf(output, "{\n  \"statementCount\": %zu,\n  \"statements\": [\n", program->count);
    for (i = 0; i < program->count; ++i) {
        const Statement *statement = program->statements[i];
        const char *kind = statement->kind == STATEMENT_ARRAY_DECLARATION
                               ? "array_declaration"
                               : (statement->kind == STATEMENT_ELEMENT_ASSIGNMENT
                                      ? "element_assignment"
                                      : "scalar_assignment");
        fprintf(output,
                "    {\"kind\": \"%s\", \"line\": %d, \"column\": %d}%s\n",
                kind,
                statement->line,
                statement->column,
                (i + 1 == program->count) ? "" : ",");
    }
    fprintf(output, "  ]\n}\n");
}
