#include "parser.h"

#include <stdlib.h>

typedef struct {
    const TokenArray *tokens;
    size_t current;
    CompilerError *error;
} Parser;

static const Token *parser_peek(const Parser *parser) {
    return &parser->tokens->items[parser->current];
}

static const Token *parser_previous(const Parser *parser) {
    return &parser->tokens->items[parser->current - 1];
}

static bool parser_is_at_end(const Parser *parser) {
    return parser_peek(parser)->type == TOKEN_EOF;
}

static const Token *parser_advance(Parser *parser) {
    if (!parser_is_at_end(parser)) {
        parser->current++;
    }
    return parser_previous(parser);
}

static bool parser_check(const Parser *parser, TokenType type) {
    if (parser_is_at_end(parser)) {
        return type == TOKEN_EOF;
    }
    return parser_peek(parser)->type == type;
}

static bool parser_match(Parser *parser, TokenType type) {
    if (!parser_check(parser, type)) {
        return false;
    }
    parser_advance(parser);
    return true;
}

static bool parser_consume(Parser *parser, TokenType type, const char *message,
                           const Token **token_out) {
    const Token *token = parser_peek(parser);
    if (parser_check(parser, type)) {
        token = parser_advance(parser);
        if (token_out != NULL) {
            *token_out = token;
        }
        return true;
    }

    compiler_error_set(parser->error, token->line, token->column,
                       "%s, found '%s'", message, token->lexeme);
    return false;
}

static Expression *make_expression(ExpressionKind kind, const Token *token) {
    Expression *expression = calloc(1, sizeof(Expression));
    if (expression == NULL) {
        return NULL;
    }
    expression->kind = kind;
    expression->line = token->line;
    expression->column = token->column;
    return expression;
}

static Statement *make_statement(StatementKind kind, const Token *token) {
    Statement *statement = calloc(1, sizeof(Statement));
    if (statement == NULL) {
        return NULL;
    }
    statement->kind = kind;
    statement->line = token->line;
    statement->column = token->column;
    return statement;
}

static bool parse_expression(Parser *parser, Expression **out_expression);

static char parse_character_value(const char *lexeme) {
    if (lexeme[1] != '\\') {
        return lexeme[1];
    }
    switch (lexeme[2]) {
        case 'n':
            return '\n';
        case 't':
            return '\t';
        case 'r':
            return '\r';
        case '0':
            return '\0';
        case '\\':
            return '\\';
        case '\'':
            return '\'';
        default:
            return lexeme[2];
    }
}

static bool parse_primary(Parser *parser, Expression **out_expression) {
    const Token *token = parser_peek(parser);

    if (parser_match(parser, TOKEN_INTEGER)) {
        Expression *expression = make_expression(EXPR_INTEGER, token);
        if (expression == NULL) {
            compiler_error_set(parser->error, token->line, token->column, "out of memory");
            return false;
        }
        expression->as.integer_value = strtol(token->lexeme, NULL, 10);
        *out_expression = expression;
        return true;
    }

    if (parser_match(parser, TOKEN_FLOAT)) {
        Expression *expression = make_expression(EXPR_FLOAT, token);
        if (expression == NULL) {
            compiler_error_set(parser->error, token->line, token->column, "out of memory");
            return false;
        }
        expression->as.float_value = strtod(token->lexeme, NULL);
        *out_expression = expression;
        return true;
    }

    if (parser_match(parser, TOKEN_CHARACTER)) {
        Expression *expression = make_expression(EXPR_CHARACTER, token);
        if (expression == NULL) {
            compiler_error_set(parser->error, token->line, token->column, "out of memory");
            return false;
        }
        expression->as.character_value = parse_character_value(token->lexeme);
        *out_expression = expression;
        return true;
    }

    if (parser_match(parser, TOKEN_IDENTIFIER)) {
        Expression *expression = NULL;

        if (parser_match(parser, TOKEN_LBRACKET)) {
            Expression *index = NULL;
            expression = make_expression(EXPR_ARRAY_ACCESS, token);
            if (expression == NULL) {
                compiler_error_set(parser->error, token->line, token->column, "out of memory");
                return false;
            }
            expression->as.array_access.array_name = compiler_string_duplicate(token->lexeme);
            if (expression->as.array_access.array_name == NULL) {
                expression_free(expression);
                compiler_error_set(parser->error, token->line, token->column, "out of memory");
                return false;
            }
            if (!parse_expression(parser, &index)) {
                expression_free(expression);
                return false;
            }
            expression->as.array_access.index = index;
            if (!parser_consume(parser, TOKEN_RBRACKET, "expected ']'", NULL)) {
                expression_free(expression);
                return false;
            }
            *out_expression = expression;
            return true;
        }

        expression = make_expression(EXPR_IDENTIFIER, token);
        if (expression == NULL) {
            compiler_error_set(parser->error, token->line, token->column, "out of memory");
            return false;
        }
        expression->as.identifier = compiler_string_duplicate(token->lexeme);
        if (expression->as.identifier == NULL) {
            expression_free(expression);
            compiler_error_set(parser->error, token->line, token->column, "out of memory");
            return false;
        }
        *out_expression = expression;
        return true;
    }

    if (parser_match(parser, TOKEN_LPAREN)) {
        Expression *inner = NULL;
        if (!parse_expression(parser, &inner)) {
            return false;
        }
        if (!parser_consume(parser, TOKEN_RPAREN, "expected ')'", NULL)) {
            expression_free(inner);
            return false;
        }
        *out_expression = inner;
        return true;
    }

    compiler_error_set(parser->error, token->line, token->column,
                       "expected expression, found '%s'", token->lexeme);
    return false;
}

static bool parse_unary(Parser *parser, Expression **out_expression) {
    const Token *token = parser_peek(parser);
    if (parser_match(parser, TOKEN_PLUS) || parser_match(parser, TOKEN_MINUS)) {
        Expression *operand = NULL;
        Expression *expression = make_expression(EXPR_UNARY, token);
        if (expression == NULL) {
            compiler_error_set(parser->error, token->line, token->column, "out of memory");
            return false;
        }
        expression->as.unary.operator = token->lexeme[0];
        if (!parse_unary(parser, &operand)) {
            expression_free(expression);
            return false;
        }
        expression->as.unary.operand = operand;
        *out_expression = expression;
        return true;
    }

    return parse_primary(parser, out_expression);
}

static bool parse_multiplication(Parser *parser, Expression **out_expression) {
    Expression *left = NULL;
    if (!parse_unary(parser, &left)) {
        return false;
    }

    while (parser_check(parser, TOKEN_STAR) || parser_check(parser, TOKEN_SLASH) ||
           parser_check(parser, TOKEN_PERCENT)) {
        const Token *operator_token = parser_advance(parser);
        Expression *right = NULL;
        Expression *binary = make_expression(EXPR_BINARY, operator_token);
        if (binary == NULL) {
            expression_free(left);
            compiler_error_set(parser->error, operator_token->line, operator_token->column,
                               "out of memory");
            return false;
        }
        binary->as.binary.operator = operator_token->lexeme[0];
        binary->as.binary.left = left;

        if (!parse_unary(parser, &right)) {
            expression_free(binary);
            return false;
        }

        binary->as.binary.right = right;
        left = binary;
    }

    *out_expression = left;
    return true;
}

static bool parse_addition(Parser *parser, Expression **out_expression) {
    Expression *left = NULL;
    if (!parse_multiplication(parser, &left)) {
        return false;
    }

    while (parser_check(parser, TOKEN_PLUS) || parser_check(parser, TOKEN_MINUS)) {
        const Token *operator_token = parser_advance(parser);
        Expression *right = NULL;
        Expression *binary = make_expression(EXPR_BINARY, operator_token);
        if (binary == NULL) {
            expression_free(left);
            compiler_error_set(parser->error, operator_token->line, operator_token->column,
                               "out of memory");
            return false;
        }
        binary->as.binary.operator = operator_token->lexeme[0];
        binary->as.binary.left = left;

        if (!parse_multiplication(parser, &right)) {
            expression_free(binary);
            return false;
        }

        binary->as.binary.right = right;
        left = binary;
    }

    *out_expression = left;
    return true;
}

static bool parse_expression(Parser *parser, Expression **out_expression) {
    return parse_addition(parser, out_expression);
}

static bool append_statement(Program *program, Statement *statement, CompilerError *error) {
    if (program->count == program->capacity) {
        size_t new_capacity = program->capacity == 0 ? 8 : program->capacity * 2;
        Statement **new_items = realloc(program->statements, new_capacity * sizeof(Statement *));
        if (new_items == NULL) {
            compiler_error_set(error, statement->line, statement->column, "out of memory");
            return false;
        }
        program->statements = new_items;
        program->capacity = new_capacity;
    }
    program->statements[program->count++] = statement;
    return true;
}

static bool append_initializer(Expression ***items, size_t *count, size_t *capacity,
                               Expression *expression, const Token *token,
                               CompilerError *error) {
    if (*count == *capacity) {
        size_t new_capacity = *capacity == 0 ? 4 : *capacity * 2;
        Expression **new_items = realloc(*items, new_capacity * sizeof(Expression *));
        if (new_items == NULL) {
            compiler_error_set(error, token->line, token->column, "out of memory");
            return false;
        }
        *items = new_items;
        *capacity = new_capacity;
    }
    (*items)[(*count)++] = expression;
    return true;
}

static bool parse_initializer(Parser *parser, Expression ***items,
                              size_t *count, size_t *capacity) {
    if (!parser_consume(parser, TOKEN_LBRACE, "expected '{'", NULL)) {
        return false;
    }

    if (parser_match(parser, TOKEN_RBRACE)) {
        return true;
    }

    while (true) {
        Expression *value = NULL;
        if (!parse_expression(parser, &value)) {
            return false;
        }
        if (!append_initializer(items, count, capacity, value,
                                parser_previous(parser), parser->error)) {
            expression_free(value);
            return false;
        }

        if (!parser_match(parser, TOKEN_COMMA)) {
            break;
        }

        if (parser_check(parser, TOKEN_RBRACE)) {
            break;
        }
    }

    return parser_consume(parser, TOKEN_RBRACE, "expected '}'", NULL);
}

static bool parse_declaration(Parser *parser, Statement **out_statement) {
    const Token *type_token = NULL;
    const Token *name_token = NULL;
    Statement *statement = NULL;

    if (!parser_consume(parser, TOKEN_TYPE, "expected type", &type_token)) {
        return false;
    }
    if (!parser_consume(parser, TOKEN_IDENTIFIER, "expected identifier", &name_token)) {
        return false;
    }

    statement = make_statement(STATEMENT_ARRAY_DECLARATION, type_token);
    if (statement == NULL) {
        compiler_error_set(parser->error, type_token->line, type_token->column, "out of memory");
        return false;
    }

    statement->as.declaration.element_type = compiler_string_duplicate(type_token->lexeme);
    statement->as.declaration.name = compiler_string_duplicate(name_token->lexeme);
    if (statement->as.declaration.element_type == NULL || statement->as.declaration.name == NULL) {
        statement_free(statement);
        compiler_error_set(parser->error, name_token->line, name_token->column, "out of memory");
        return false;
    }

    if (!parser_consume(parser, TOKEN_LBRACKET, "expected '['", NULL)) {
        statement_free(statement);
        return false;
    }

    if (!parser_check(parser, TOKEN_RBRACKET)) {
        if (!parse_expression(parser, &statement->as.declaration.size)) {
            statement_free(statement);
            return false;
        }
    }

    if (!parser_consume(parser, TOKEN_RBRACKET, "expected ']'", NULL)) {
        statement_free(statement);
        return false;
    }

    if (parser_match(parser, TOKEN_ASSIGN)) {
        size_t initializer_capacity = 0;
        statement->as.declaration.has_initializer = true;
        if (!parse_initializer(parser,
                               &statement->as.declaration.initializer,
                               &statement->as.declaration.initializer_count,
                               &initializer_capacity)) {
            statement_free(statement);
            return false;
        }
    }

    if (statement->as.declaration.size == NULL && !statement->as.declaration.has_initializer) {
        compiler_error_set(parser->error, name_token->line, name_token->column,
                           "unsized declaration requires an initializer");
        statement_free(statement);
        return false;
    }

    if (!parser_consume(parser, TOKEN_SEMICOLON, "expected ';'", NULL)) {
        statement_free(statement);
        return false;
    }

    *out_statement = statement;
    return true;
}

static bool parse_assignment(Parser *parser, Statement **out_statement) {
    const Token *name_token = NULL;
    Statement *statement = NULL;

    if (!parser_consume(parser, TOKEN_IDENTIFIER, "expected identifier", &name_token)) {
        return false;
    }

    if (parser_match(parser, TOKEN_LBRACKET)) {
        statement = make_statement(STATEMENT_ELEMENT_ASSIGNMENT, name_token);
        if (statement == NULL) {
            compiler_error_set(parser->error, name_token->line, name_token->column, "out of memory");
            return false;
        }
        statement->as.assignment.array_name = compiler_string_duplicate(name_token->lexeme);
        if (statement->as.assignment.array_name == NULL) {
            statement_free(statement);
            compiler_error_set(parser->error, name_token->line, name_token->column, "out of memory");
            return false;
        }

        if (!parse_expression(parser, &statement->as.assignment.index)) {
            statement_free(statement);
            return false;
        }
        if (!parser_consume(parser, TOKEN_RBRACKET, "expected ']'", NULL)) {
            statement_free(statement);
            return false;
        }

        if (!parser_consume(parser, TOKEN_ASSIGN, "expected '='", NULL)) {
            statement_free(statement);
            return false;
        }

        if (!parse_expression(parser, &statement->as.assignment.value)) {
            statement_free(statement);
            return false;
        }
    } else {
        statement = make_statement(STATEMENT_SCALAR_ASSIGNMENT, name_token);
        if (statement == NULL) {
            compiler_error_set(parser->error, name_token->line, name_token->column, "out of memory");
            return false;
        }
        statement->as.scalar_assignment.name = compiler_string_duplicate(name_token->lexeme);
        if (statement->as.scalar_assignment.name == NULL) {
            statement_free(statement);
            compiler_error_set(parser->error, name_token->line, name_token->column, "out of memory");
            return false;
        }

        if (!parser_consume(parser, TOKEN_ASSIGN, "expected '='", NULL)) {
            statement_free(statement);
            return false;
        }

        if (!parse_expression(parser, &statement->as.scalar_assignment.value)) {
            statement_free(statement);
            return false;
        }
    }

    if (!parser_consume(parser, TOKEN_SEMICOLON, "expected ';'", NULL)) {
        statement_free(statement);
        return false;
    }

    *out_statement = statement;
    return true;
}

bool parser_parse(const TokenArray *tokens, Program *program, CompilerError *error) {
    Parser parser;

    compiler_error_clear(error);
    program->statements = NULL;
    program->count = 0;
    program->capacity = 0;

    parser.tokens = tokens;
    parser.current = 0;
    parser.error = error;

    while (!parser_is_at_end(&parser)) {
        Statement *statement = NULL;
        const Token *token = parser_peek(&parser);

        if (token->type == TOKEN_TYPE) {
            if (!parse_declaration(&parser, &statement)) {
                program_free(program);
                return false;
            }
        } else if (token->type == TOKEN_IDENTIFIER) {
            if (!parse_assignment(&parser, &statement)) {
                program_free(program);
                return false;
            }
        } else {
            compiler_error_set(error, token->line, token->column,
                               "expected statement, found '%s'", token->lexeme);
            program_free(program);
            return false;
        }

        if (!append_statement(program, statement, error)) {
            statement_free(statement);
            program_free(program);
            return false;
        }
    }

    return true;
}
