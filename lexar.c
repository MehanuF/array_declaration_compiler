
#include <stdio.h>
#include <ctype.h>
#include <string.h>

/* Array Declaration Compiler
   Member 2 - Lexical Analyzer */

#define MAX_LEXEME 100

typedef enum {
    TOK_INT,
    TOK_FLOAT,
    TOK_CHAR,
    TOK_ID,
    TOK_INT_LITERAL,
    TOK_ASSIGN,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_SEMICOLON,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_COMMA,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MULTIPLY,
    TOK_DIVIDE,
    TOK_EOF,
    TOK_INVALID
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[MAX_LEXEME];
    int line;
    int column;
} Token;

/* Convert token type to readable name */
const char *tokenName(TokenType type) {
    switch (type) {
        case TOK_INT:         return "INT";
        case TOK_FLOAT:       return "FLOAT";
        case TOK_CHAR:        return "CHAR";
        case TOK_ID:          return "ID";
        case TOK_INT_LITERAL: return "INT_LITERAL";
        case TOK_ASSIGN:      return "ASSIGN";
        case TOK_LBRACKET:    return "LBRACKET";
        case TOK_RBRACKET:    return "RBRACKET";
        case TOK_LBRACE:      return "LBRACE";
        case TOK_RBRACE:      return "RBRACE";
        case TOK_SEMICOLON:   return "SEMICOLON";
        case TOK_LPAREN:      return "LPAREN";
        case TOK_RPAREN:      return "RPAREN";
        case TOK_COMMA:       return "COMMA";
        case TOK_PLUS:        return "PLUS";
        case TOK_MINUS:       return "MINUS";
        case TOK_MULTIPLY:    return "MULTIPLY";
        case TOK_DIVIDE:      return "DIVIDE";
        case TOK_EOF:          return "EOF";
        default:               return "INVALID";
    }
}

/* Check whether a word is a keyword */
TokenType checkKeyword(const char *word) {
    if (strcmp(word, "int") == 0)
        return TOK_INT;
    if (strcmp(word, "float") == 0)
        return TOK_FLOAT;
    if (strcmp(word, "char") == 0)
        return TOK_CHAR;

    return TOK_ID;
}

/* Create and print a token */
void printToken(TokenType type, const char *lexeme,
                int line, int column) {
    printf("%-15s %-12s Line: %d, Column: %d\n",
           tokenName(type), lexeme, line, column);
}

/* Perform lexical analysis */
void tokenize(const char *source) {
    int i = 0;
    int line = 1;
    int column = 1;

    while (source[i] != '\0') {
        char ch = source[i];

        /* Ignore whitespace */
        if (isspace((unsigned char)ch)) {
            if (ch == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            i++;
            continue;
        }

        int startLine = line;
        int startColumn = column;

        /* Recognize identifiers and keywords */
        if (isalpha((unsigned char)ch) || ch == '_') {
            char word[MAX_LEXEME];
            int j = 0;

            while (isalnum((unsigned char)source[i]) ||
                   source[i] == '_') {
                if (j < MAX_LEXEME - 1)
                    word[j++] = source[i];

                i++;
                column++;
            }

            word[j] = '\0';

            TokenType type = checkKeyword(word);
            printToken(type, word, startLine, startColumn);
            continue;
        }

        /* Recognize integer literals */
        if (isdigit((unsigned char)ch)) {
            char number[MAX_LEXEME];
            int j = 0;

            while (isdigit((unsigned char)source[i])) {
                if (j < MAX_LEXEME - 1)
                    number[j++] = source[i];

                i++;
                column++;
            }

            number[j] = '\0';

            /* Decimal literals are not supported */
            if (source[i] == '.' &&
                isdigit((unsigned char)source[i + 1])) {
                printf("Lexical Error: Decimal literal "
                       "not supported at line %d, column %d\n",
                       line, column);
                return;
            }

            printToken(TOK_INT_LITERAL, number,
                       startLine, startColumn);
            continue;
        }

        /* Recognize operators and delimiters */
        TokenType type;

        switch (ch) {
            case '=': type = TOK_ASSIGN; break;
            case '[': type = TOK_LBRACKET; break;
            case ']': type = TOK_RBRACKET; break;
            case '{': type = TOK_LBRACE; break;
            case '}': type = TOK_RBRACE; break;
            case ';': type = TOK_SEMICOLON; break;
            case '(': type = TOK_LPAREN; break;
            case ')': type = TOK_RPAREN; break;
            case ',': type = TOK_COMMA; break;
            case '+': type = TOK_PLUS; break;
            case '-': type = TOK_MINUS; break;
            case '*': type = TOK_MULTIPLY; break;
            case '/': type = TOK_DIVIDE; break;

            default:
                printf("Lexical Error: Invalid character "
                       "'%c' at line %d, column %d\n",
                       ch, line, column);
                return;
        }

        char symbol[2] = {ch, '\0'};
        printToken(type, symbol, startLine, startColumn);

        i++;
        column++;
    }

    printToken(TOK_EOF, "EOF", line, column);
}

int main(void) {
    char source[5000];
    int ch;
    size_t length = 0;

    printf("ARRAY DECLARATION COMPILER\n");
    printf("Enter source code (Ctrl+D on Linux/Mac, "
           "Ctrl+Z then Enter on Windows to finish):\n\n");

    while ((ch = getchar()) != EOF) {
        if (length >= sizeof(source) - 1) {
            printf("Error: Input is too long.\n");
            return 1;
        }

        source[length++] = (char)ch;
    }

    source[length] = '\0';

    printf("\nTOKEN OUTPUT\n");
    printf("-----------------------------------------------\n");
    tokenize(source);

    return 0;
}
