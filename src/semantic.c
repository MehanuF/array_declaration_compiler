#include "semantic.h"

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    VALUE_INT,
    VALUE_FLOAT,
    VALUE_CHAR,
    VALUE_UNKNOWN
} ValueType;

typedef struct {
    const char *name;
    ValueType element_type;
    long size;
} Symbol;

typedef struct {
    Symbol entries[256];
    size_t count;
} SymbolTable;

typedef struct {
    ValueType type;
    bool is_int_constant;
    long int_constant;
} ExpressionInfo;

static ValueType element_type_from_name(const char *name) {
    if (strcmp(name, "int") == 0) {
        return VALUE_INT;
    }
    if (strcmp(name, "float") == 0) {
        return VALUE_FLOAT;
    }
    if (strcmp(name, "char") == 0) {
        return VALUE_CHAR;
    }
    return VALUE_UNKNOWN;
}

static const char *value_type_name(ValueType type) {
    switch (type) {
        case VALUE_INT:
            return "int";
        case VALUE_FLOAT:
            return "float";
        case VALUE_CHAR:
            return "char";
        default:
            return "unknown";
    }
}

static bool assignment_allowed(ValueType target, ValueType source) {
    if (target == source) {
        return true;
    }
    if (target == VALUE_FLOAT && (source == VALUE_INT || source == VALUE_CHAR)) {
        return true;
    }
    if (target == VALUE_INT && source == VALUE_CHAR) {
        return true;
    }
    if (target == VALUE_CHAR && source == VALUE_INT) {
        return true;
    }
    return false;
}

static Symbol *find_symbol(SymbolTable *table, const char *name) {
    size_t i;
    for (i = 0; i < table->count; ++i) {
        if (strcmp(table->entries[i].name, name) == 0) {
            return &table->entries[i];
        }
    }
    return NULL;
}

static bool set_semantic_error(CompilerError *error, int line, int column, const char *format, ...) {
    va_list arguments;

    if (error->occurred) {
        return false;
    }

    error->occurred = true;
    error->line = line;
    error->column = column;

    va_start(arguments, format);
    vsnprintf(error->message, sizeof(error->message), format, arguments);
    va_end(arguments);

    return false;
}

static bool infer_expression(const Expression *expression,
                             SymbolTable *table,
                             ExpressionInfo *out,
                             CompilerError *error);

static bool infer_binary_int_constant(char operator, long left, long right,
                                      long *result, CompilerError *error,
                                      int line, int column) {
    switch (operator) {
        case '+':
            *result = left + right;
            return true;
        case '-':
            *result = left - right;
            return true;
        case '*':
            *result = left * right;
            return true;
        case '/':
            if (right == 0) {
                return set_semantic_error(error, line, column, "division by zero in constant expression");
            }
            *result = left / right;
            return true;
        case '%':
            if (right == 0) {
                return set_semantic_error(error, line, column, "modulo by zero in constant expression");
            }
            *result = left % right;
            return true;
        default:
            return false;
    }
}

static bool infer_expression(const Expression *expression,
                             SymbolTable *table,
                             ExpressionInfo *out,
                             CompilerError *error) {
    ExpressionInfo left;
    ExpressionInfo right;

    out->type = VALUE_UNKNOWN;
    out->is_int_constant = false;
    out->int_constant = 0;

    switch (expression->kind) {
        case EXPR_INTEGER:
            out->type = VALUE_INT;
            out->is_int_constant = true;
            out->int_constant = expression->as.integer_value;
            return true;
        case EXPR_FLOAT:
            out->type = VALUE_FLOAT;
            return true;
        case EXPR_CHARACTER:
            out->type = VALUE_CHAR;
            out->is_int_constant = true;
            out->int_constant = (unsigned char)expression->as.character_value;
            return true;
        case EXPR_IDENTIFIER: {
            Symbol *symbol = find_symbol(table, expression->as.identifier);
            if (symbol == NULL) {
                return set_semantic_error(error, expression->line, expression->column,
                                          "undeclared identifier '%s'", expression->as.identifier);
            }
            out->type = symbol->element_type;
            return true;
        }
        case EXPR_ARRAY_ACCESS: {
            Symbol *symbol = find_symbol(table, expression->as.array_access.array_name);
            ExpressionInfo index_info;
            if (symbol == NULL) {
                return set_semantic_error(error, expression->line, expression->column,
                                          "undeclared array '%s'", expression->as.array_access.array_name);
            }
            if (!infer_expression(expression->as.array_access.index, table, &index_info, error)) {
                return false;
            }
            if (index_info.type != VALUE_INT) {
                return set_semantic_error(error, expression->line, expression->column,
                                          "array index for '%s' must be integer",
                                          expression->as.array_access.array_name);
            }
            if (index_info.is_int_constant &&
                (index_info.int_constant < 0 || index_info.int_constant >= symbol->size)) {
                return set_semantic_error(error, expression->line, expression->column,
                                          "index %ld out of bounds for array '%s' of size %ld",
                                          index_info.int_constant,
                                          expression->as.array_access.array_name,
                                          symbol->size);
            }
            out->type = symbol->element_type;
            return true;
        }
        case EXPR_UNARY:
            if (!infer_expression(expression->as.unary.operand, table, &left, error)) {
                return false;
            }
            if (left.type != VALUE_INT && left.type != VALUE_FLOAT && left.type != VALUE_CHAR) {
                return set_semantic_error(error, expression->line, expression->column,
                                          "invalid unary operand type");
            }
            out->type = left.type == VALUE_CHAR ? VALUE_INT : left.type;
            if (left.is_int_constant) {
                out->is_int_constant = true;
                out->int_constant = expression->as.unary.operator == '-' ? -left.int_constant : left.int_constant;
            }
            return true;
        case EXPR_BINARY:
            if (!infer_expression(expression->as.binary.left, table, &left, error)) {
                return false;
            }
            if (!infer_expression(expression->as.binary.right, table, &right, error)) {
                return false;
            }
            if (left.type == VALUE_UNKNOWN || right.type == VALUE_UNKNOWN) {
                return false;
            }
            if (expression->as.binary.operator == '%' &&
                (left.type != VALUE_INT || right.type != VALUE_INT)) {
                return set_semantic_error(error, expression->line, expression->column,
                                          "'%%' requires integer operands");
            }
            if (left.type == VALUE_FLOAT || right.type == VALUE_FLOAT) {
                out->type = VALUE_FLOAT;
            } else {
                out->type = VALUE_INT;
            }
            if (left.is_int_constant && right.is_int_constant &&
                out->type == VALUE_INT) {
                long result;
                if (!infer_binary_int_constant(expression->as.binary.operator,
                                               left.int_constant,
                                               right.int_constant,
                                               &result,
                                               error,
                                               expression->line,
                                               expression->column)) {
                    return false;
                }
                out->is_int_constant = true;
                out->int_constant = result;
            }
            return true;
    }

    return false;
}

static bool validate_declaration(const Statement *statement,
                                 SymbolTable *table,
                                 CompilerError *error) {
    Symbol *existing;
    Symbol *symbol;
    ValueType declared_type = element_type_from_name(statement->as.declaration.element_type);
    long declared_size = 0;
    bool has_known_size = false;
    size_t i;

    existing = find_symbol(table, statement->as.declaration.name);
    if (existing != NULL) {
        return set_semantic_error(error, statement->line, statement->column,
                                  "duplicate declaration of '%s'",
                                  statement->as.declaration.name);
    }

    if (declared_type == VALUE_UNKNOWN) {
        return set_semantic_error(error, statement->line, statement->column,
                                  "unknown type '%s'", statement->as.declaration.element_type);
    }

    if (statement->as.declaration.size != NULL) {
        ExpressionInfo size_info;
        if (!infer_expression(statement->as.declaration.size, table, &size_info, error)) {
            return false;
        }
        if (size_info.type != VALUE_INT || !size_info.is_int_constant) {
            return set_semantic_error(error,
                                      statement->as.declaration.size->line,
                                      statement->as.declaration.size->column,
                                      "array size must be an integer constant expression");
        }
        if (size_info.int_constant <= 0) {
            return set_semantic_error(error,
                                      statement->as.declaration.size->line,
                                      statement->as.declaration.size->column,
                                      "array '%s' size must be positive",
                                      statement->as.declaration.name);
        }
        declared_size = size_info.int_constant;
        has_known_size = true;
    }

    if (statement->as.declaration.has_initializer) {
        if (has_known_size && statement->as.declaration.initializer_count > (size_t)declared_size) {
            return set_semantic_error(error, statement->line, statement->column,
                                      "initializer count exceeds declared size for '%s'",
                                      statement->as.declaration.name);
        }

        for (i = 0; i < statement->as.declaration.initializer_count; ++i) {
            ExpressionInfo value_info;
            if (!infer_expression(statement->as.declaration.initializer[i], table, &value_info, error)) {
                return false;
            }
            if (!assignment_allowed(declared_type, value_info.type)) {
                return set_semantic_error(error,
                                          statement->as.declaration.initializer[i]->line,
                                          statement->as.declaration.initializer[i]->column,
                                          "initializer type '%s' is not compatible with '%s'",
                                          value_type_name(value_info.type),
                                          value_type_name(declared_type));
            }
        }
    }

    if (!has_known_size) {
        declared_size = (long)statement->as.declaration.initializer_count;
        if (declared_size <= 0) {
            return set_semantic_error(error, statement->line, statement->column,
                                      "unsized declaration of '%s' requires at least one initializer",
                                      statement->as.declaration.name);
        }
    }

    if (table->count >= sizeof(table->entries) / sizeof(table->entries[0])) {
        return set_semantic_error(error, statement->line, statement->column,
                                  "symbol table capacity exceeded");
    }

    symbol = &table->entries[table->count++];
    symbol->name = statement->as.declaration.name;
    symbol->element_type = declared_type;
    symbol->size = declared_size;

    return true;
}

static bool validate_assignment(const Statement *statement,
                                SymbolTable *table,
                                CompilerError *error) {
    ExpressionInfo value_info;
    ValueType target_type = VALUE_UNKNOWN;

    if (statement->kind == STATEMENT_ELEMENT_ASSIGNMENT) {
        Symbol *symbol = find_symbol(table, statement->as.assignment.array_name);
        ExpressionInfo index_info;

        if (symbol == NULL) {
            return set_semantic_error(error, statement->line, statement->column,
                                      "undeclared array '%s'", statement->as.assignment.array_name);
        }

        if (!infer_expression(statement->as.assignment.index, table, &index_info, error)) {
            return false;
        }

        if (index_info.type != VALUE_INT) {
            return set_semantic_error(error,
                                      statement->as.assignment.index->line,
                                      statement->as.assignment.index->column,
                                      "array index for '%s' must be integer",
                                      statement->as.assignment.array_name);
        }

        if (index_info.is_int_constant &&
            (index_info.int_constant < 0 || index_info.int_constant >= symbol->size)) {
            return set_semantic_error(error,
                                      statement->as.assignment.index->line,
                                      statement->as.assignment.index->column,
                                      "index %ld out of bounds for array '%s' of size %ld",
                                      index_info.int_constant,
                                      statement->as.assignment.array_name,
                                      symbol->size);
        }

        target_type = symbol->element_type;
        if (!infer_expression(statement->as.assignment.value, table, &value_info, error)) {
            return false;
        }
    } else {
        Symbol *symbol = find_symbol(table, statement->as.scalar_assignment.name);
        if (symbol == NULL) {
            return set_semantic_error(error, statement->line, statement->column,
                                      "undeclared identifier '%s'",
                                      statement->as.scalar_assignment.name);
        }
        target_type = symbol->element_type;
        if (!infer_expression(statement->as.scalar_assignment.value, table, &value_info, error)) {
            return false;
        }
    }

    if (!assignment_allowed(target_type, value_info.type)) {
        return set_semantic_error(error, statement->line, statement->column,
                                  "cannot assign '%s' to '%s'",
                                  value_type_name(value_info.type),
                                  value_type_name(target_type));
    }

    return true;
}

bool semantic_analyze(const Program *program, CompilerError *error) {
    SymbolTable table;
    size_t i;

    compiler_error_clear(error);
    memset(&table, 0, sizeof(table));

    for (i = 0; i < program->count; ++i) {
        const Statement *statement = program->statements[i];
        if (statement->kind == STATEMENT_ARRAY_DECLARATION) {
            if (!validate_declaration(statement, &table, error)) {
                return false;
            }
        } else {
            if (!validate_assignment(statement, &table, error)) {
                return false;
            }
        }
    }

    return true;
}
