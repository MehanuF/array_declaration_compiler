#include "parser.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    const TokenArray *tokens;
    size_t current;
    CompilerError *error;
} Parser;

static const Token *peek(const Parser *parser) {
    return &parser->tokens->items[parser->current];
}

static const Token *previous(const Parser *parser) {
    return &parser->tokens->items[parser->current - 1];
}

static bool check(const Parser *parser, TokenType type) {
    return peek(parser)->type == type;
}

static const Token *advance(Parser *parser) {
    if (!check(parser, TOKEN_EOF)) {
        parser->current++;
    }
    return previous(parser);
}

static const Token *consume(Parser *parser, TokenType type, const char *message) {
    const Token *token = peek(parser);
    if (token->type == type) {
        return advance(parser);
    }
    compiler_error_set(parser->error, token->line, token->column, "%s%s", message,
                       token->type == TOKEN_EOF ? " before end of input" : "");
    return NULL;
}

static Expression *new_expression(Parser *parser, ExpressionKind kind,
                                  const Token *token) {
    Expression *expression = calloc(1, sizeof(*expression));
    if (expression == NULL) {
        compiler_error_set(parser->error, token->line, token->column,
                           "out of memory while building AST");
        return NULL;
    }
    expression->kind = kind;
    expression->line = token->line;
    expression->column = token->column;
    return expression;
}

static char *duplicate_lexeme(Parser *parser, const Token *token) {
    char *copy = compiler_string_duplicate(token->lexeme);
    if (copy == NULL) {
        compiler_error_set(parser->error, token->line, token->column,
                           "out of memory while building AST");
    }
    return copy;
}

static Expression *parse_expression(Parser *parser);

static Expression *parse_array_access(Parser *parser, const Token *name) {
    Expression *access;
    Expression *index;

    if (consume(parser, TOKEN_LBRACKET, "expected '[' after array name") == NULL) {
        return NULL;
    }
    index = parse_expression(parser);
    if (index == NULL) {
        return NULL;
    }
    if (consume(parser, TOKEN_RBRACKET, "expected ']' after array index") == NULL) {
        expression_free(index);
        return NULL;
    }
    access = new_expression(parser, EXPR_ARRAY_ACCESS, name);
    if (access == NULL) {
        expression_free(index);
        return NULL;
    }
    access->as.array_access.array_name = duplicate_lexeme(parser, name);
    access->as.array_access.index = index;
    if (access->as.array_access.array_name == NULL) {
        expression_free(access);
        return NULL;
    }
    return access;
}

static char decode_character(const char *lexeme) {
    if (lexeme[1] != '\\') {
        return lexeme[1];
    }
    switch (lexeme[2]) {
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        case '0': return '\0';
        case '\\': return '\\';
        case '\'': return '\'';
        default: return lexeme[2];
    }
}

static Expression *parse_primary(Parser *parser) {
    const Token *token = peek(parser);
    Expression *expression;

    if (check(parser, TOKEN_INTEGER)) {
        advance(parser);
        expression = new_expression(parser, EXPR_INTEGER, token);
        if (expression != NULL) {
            expression->as.integer_value = strtol(token->lexeme, NULL, 10);
        }
        return expression;
    }
    if (check(parser, TOKEN_FLOAT)) {
        advance(parser);
        expression = new_expression(parser, EXPR_FLOAT, token);
        if (expression != NULL) {
            expression->as.float_value = strtod(token->lexeme, NULL);
        }
        return expression;
    }
    if (check(parser, TOKEN_CHARACTER)) {
        advance(parser);
        expression = new_expression(parser, EXPR_CHARACTER, token);
        if (expression != NULL) {
            expression->as.character_value = decode_character(token->lexeme);
        }
        return expression;
    }
    if (check(parser, TOKEN_IDENTIFIER)) {
        advance(parser);
        if (check(parser, TOKEN_LBRACKET)) {
            return parse_array_access(parser, token);
        }
        expression = new_expression(parser, EXPR_IDENTIFIER, token);
        if (expression != NULL) {
            expression->as.identifier = duplicate_lexeme(parser, token);
            if (expression->as.identifier == NULL) {
                expression_free(expression);
                return NULL;
            }
        }
        return expression;
    }
    if (check(parser, TOKEN_LPAREN)) {
        advance(parser);
        expression = parse_expression(parser);
        if (expression == NULL) {
            return NULL;
        }
        if (consume(parser, TOKEN_RPAREN, "expected ')' after expression") == NULL) {
            expression_free(expression);
            return NULL;
        }
        return expression;
    }

    compiler_error_set(parser->error, token->line, token->column,
                       "expected an expression%s",
                       token->type == TOKEN_EOF ? " before end of input" : "");
    return NULL;
}

static Expression *parse_unary(Parser *parser) {
    if (check(parser, TOKEN_PLUS) || check(parser, TOKEN_MINUS)) {
        const Token *operator_token = advance(parser);
        Expression *operand = parse_unary(parser);
        Expression *expression;
        if (operand == NULL) {
            return NULL;
        }
        expression = new_expression(parser, EXPR_UNARY, operator_token);
        if (expression == NULL) {
            expression_free(operand);
            return NULL;
        }
        expression->as.unary.operator = operator_token->lexeme[0];
        expression->as.unary.operand = operand;
        return expression;
    }
    return parse_primary(parser);
}

static Expression *parse_multiplication(Parser *parser) {
    Expression *left = parse_unary(parser);
    while (left != NULL && (check(parser, TOKEN_STAR) || check(parser, TOKEN_SLASH) ||
                            check(parser, TOKEN_PERCENT))) {
        const Token *operator_token = advance(parser);
        Expression *right = parse_unary(parser);
        Expression *combined;
        if (right == NULL) {
            expression_free(left);
            return NULL;
        }
        combined = new_expression(parser, EXPR_BINARY, operator_token);
        if (combined == NULL) {
            expression_free(left);
            expression_free(right);
            return NULL;
        }
        combined->as.binary.left = left;
        combined->as.binary.operator = operator_token->lexeme[0];
        combined->as.binary.right = right;
        left = combined;
    }
    return left;
}

static Expression *parse_addition(Parser *parser) {
    Expression *left = parse_multiplication(parser);
    while (left != NULL && (check(parser, TOKEN_PLUS) || check(parser, TOKEN_MINUS))) {
        const Token *operator_token = advance(parser);
        Expression *right = parse_multiplication(parser);
        Expression *combined;
        if (right == NULL) {
            expression_free(left);
            return NULL;
        }
        combined = new_expression(parser, EXPR_BINARY, operator_token);
        if (combined == NULL) {
            expression_free(left);
            expression_free(right);
            return NULL;
        }
        combined->as.binary.left = left;
        combined->as.binary.operator = operator_token->lexeme[0];
        combined->as.binary.right = right;
        left = combined;
    }
    return left;
}

static Expression *parse_expression(Parser *parser) {
    return parse_addition(parser);
}

static bool append_initializer(Parser *parser, Statement *statement, Expression *value) {
    size_t count = statement->as.declaration.initializer_count;
    Expression **larger = realloc(statement->as.declaration.initializer,
                                  (count + 1) * sizeof(*larger));
    if (larger == NULL) {
        compiler_error_set(parser->error, value->line, value->column,
                           "out of memory while building initializer");
        expression_free(value);
        return false;
    }
    statement->as.declaration.initializer = larger;
    statement->as.declaration.initializer[count] = value;
    statement->as.declaration.initializer_count++;
    return true;
}

static Statement *parse_declaration(Parser *parser) {
    const Token *type_token = advance(parser);
    const Token *name = consume(parser, TOKEN_IDENTIFIER, "expected array name after type");
    Statement *statement;

    if (name == NULL || consume(parser, TOKEN_LBRACKET, "expected '[' after array name") == NULL) {
        return NULL;
    }
    statement = calloc(1, sizeof(*statement));
    if (statement == NULL) {
        compiler_error_set(parser->error, type_token->line, type_token->column,
                           "out of memory while building AST");
        return NULL;
    }
    statement->kind = STATEMENT_ARRAY_DECLARATION;
    statement->line = type_token->line;
    statement->column = type_token->column;
    statement->as.declaration.element_type = duplicate_lexeme(parser, type_token);
    statement->as.declaration.name = duplicate_lexeme(parser, name);
    if (statement->as.declaration.element_type == NULL || statement->as.declaration.name == NULL) {
        statement_free(statement);
        return NULL;
    }

    if (!check(parser, TOKEN_RBRACKET)) {
        statement->as.declaration.size = parse_expression(parser);
        if (statement->as.declaration.size == NULL) {
            statement_free(statement);
            return NULL;
        }
    }
    if (consume(parser, TOKEN_RBRACKET, "expected ']' after array size") == NULL) {
        statement_free(statement);
        return NULL;
    }

    if (check(parser, TOKEN_ASSIGN)) {
        advance(parser);
        statement->as.declaration.has_initializer = true;
        if (consume(parser, TOKEN_LBRACE, "expected '{' to start array initializer") == NULL) {
            statement_free(statement);
            return NULL;
        }
        if (!check(parser, TOKEN_RBRACE)) {
            for (;;) {
                Expression *value = parse_expression(parser);
                if (value == NULL || !append_initializer(parser, statement, value)) {
                    statement_free(statement);
                    return NULL;
                }
                if (!check(parser, TOKEN_COMMA)) {
                    break;
                }
                advance(parser);
                if (check(parser, TOKEN_RBRACE)) {
                    break;
                }
            }
        }
        if (consume(parser, TOKEN_RBRACE, "expected '}' after array initializer") == NULL) {
            statement_free(statement);
            return NULL;
        }
    }

    if (statement->as.declaration.size == NULL &&
        !statement->as.declaration.has_initializer) {
        compiler_error_set(parser->error, name->line, name->column,
                           "an array with omitted size requires an initializer");
        statement_free(statement);
        return NULL;
    }
    if (consume(parser, TOKEN_SEMICOLON, "expected ';' after array declaration") == NULL) {
        statement_free(statement);
        return NULL;
    }
    return statement;
}

static Statement *parse_assignment(Parser *parser) {
    const Token *name = advance(parser);
    Expression *target;
    Statement *statement;

    if (!check(parser, TOKEN_LBRACKET)) {
        if (consume(parser, TOKEN_ASSIGN,
                    "expected '[' or '=' after assignment target") == NULL) {
            return NULL;
        }
        statement = calloc(1, sizeof(*statement));
        if (statement == NULL) {
            compiler_error_set(parser->error, name->line, name->column,
                               "out of memory while building AST");
            return NULL;
        }
        statement->kind = STATEMENT_SCALAR_ASSIGNMENT;
        statement->line = name->line;
        statement->column = name->column;
        statement->as.scalar_assignment.name = duplicate_lexeme(parser, name);
        if (statement->as.scalar_assignment.name == NULL) {
            statement_free(statement);
            return NULL;
        }
        statement->as.scalar_assignment.value = parse_expression(parser);
        if (statement->as.scalar_assignment.value == NULL ||
            consume(parser, TOKEN_SEMICOLON, "expected ';' after assignment") == NULL) {
            statement_free(statement);
            return NULL;
        }
        return statement;
    }

    target = parse_array_access(parser, name);
    if (target == NULL) {
        return NULL;
    }
    if (consume(parser, TOKEN_ASSIGN, "expected '=' after indexed array element") == NULL) {
        expression_free(target);
        return NULL;
    }
    statement = calloc(1, sizeof(*statement));
    if (statement == NULL) {
        compiler_error_set(parser->error, name->line, name->column,
                           "out of memory while building AST");
        expression_free(target);
        return NULL;
    }
    statement->kind = STATEMENT_ELEMENT_ASSIGNMENT;
    statement->line = name->line;
    statement->column = name->column;
    statement->as.assignment.array_name = target->as.array_access.array_name;
    statement->as.assignment.index = target->as.array_access.index;
    target->as.array_access.array_name = NULL;
    target->as.array_access.index = NULL;
    expression_free(target);

    statement->as.assignment.value = parse_expression(parser);
    if (statement->as.assignment.value == NULL ||
        consume(parser, TOKEN_SEMICOLON, "expected ';' after element assignment") == NULL) {
        statement_free(statement);
        return NULL;
    }
    return statement;
}

static bool append_statement(Parser *parser, Program *program, Statement *statement) {
    Statement **larger;
    if (program->count == program->capacity) {
        size_t capacity = program->capacity == 0 ? 8 : program->capacity * 2;
        larger = realloc(program->statements, capacity * sizeof(*larger));
        if (larger == NULL) {
            compiler_error_set(parser->error, statement->line, statement->column,
                               "out of memory while building program");
            statement_free(statement);
            return false;
        }
        program->statements = larger;
        program->capacity = capacity;
    }
    program->statements[program->count++] = statement;
    return true;
}

bool parser_parse(const TokenArray *tokens, Program *program, CompilerError *error) {
    Parser parser = {tokens, 0, error};
    compiler_error_clear(error);
    program->statements = NULL;
    program->count = 0;
    program->capacity = 0;

    while (!check(&parser, TOKEN_EOF)) {
        Statement *statement;
        if (check(&parser, TOKEN_TYPE)) {
            statement = parse_declaration(&parser);
        } else if (check(&parser, TOKEN_IDENTIFIER)) {
            statement = parse_assignment(&parser);
        } else {
            const Token *token = peek(&parser);
            compiler_error_set(error, token->line, token->column,
                               "expected an array declaration or element assignment");
            program_free(program);
            return false;
        }
        if (statement == NULL || !append_statement(&parser, program, statement)) {
            program_free(program);
            return false;
        }
    }
    return true;
}
