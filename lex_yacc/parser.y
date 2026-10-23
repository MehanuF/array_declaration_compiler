/*
 * parser.y  —  YACC/Bison specification for the Array Declaration Compiler
 * Member 3 — Syntax Analyzer
 *
 * Grammar: LALR(1) — YACC uses bottom-up (shift-reduce) parsing.
 * Note: YACC accepts left-recursive rules natively (unlike LL parsers).
 *
 * Compile:
 *   bison -d -o lex_yacc/parser.tab.c lex_yacc/parser.y
 *   flex  -o lex_yacc/lex.yy.c lex_yacc/lexer.l
 *   gcc lex_yacc/parser.tab.c lex_yacc/lex.yy.c -o array_compiler_ly -lfl
 *
 * Or just run:
 *   make lex-yacc
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Forward declarations ─────────────────────────────────── */
int  yylex(void);
void yyerror(const char *msg);

extern int line_num;          /* defined in lex.yy.c */

/* ── Simple symbol table for semantic checks ────────────── */
#define MAX_SYM 64
typedef struct {
    char name[64];
    char type[8];
    int  size;
    int  line;
} Sym;
static Sym symtab[MAX_SYM];
static int sym_count = 0;
static int error_count = 0;

static Sym *sym_lookup(const char *name) {
    for (int i = 0; i < sym_count; i++)
        if (strcmp(symtab[i].name, name) == 0) return &symtab[i];
    return NULL;
}
static void sym_insert(const char *name, const char *type, int size, int line) {
    if (sym_lookup(name)) {
        fprintf(stderr,
            "Semantic Error [line %d]: array '%s' already declared\n",
            line, name);
        error_count++;
        return;
    }
    if (sym_count >= MAX_SYM) { fprintf(stderr, "Symbol table full\n"); return; }
    strncpy(symtab[sym_count].name, name, 63);
    strncpy(symtab[sym_count].type, type,  7);
    symtab[sym_count].size = size;
    symtab[sym_count].line = line;
    sym_count++;
}
static void print_symtab(void) {
    printf("\n  Symbol Table:\n");
    printf("  %-20s %-8s %-6s %s\n", "Name", "Type", "Size", "Declared");
    printf("  %-20s %-8s %-6s %s\n",
           "--------------------","--------","------","--------");
    if (sym_count == 0) { printf("  (empty)\n"); return; }
    for (int i = 0; i < sym_count; i++)
        printf("  %-20s %-8s %-6d line %d\n",
               symtab[i].name, symtab[i].type, symtab[i].size, symtab[i].line);
}
%}

/* ── Value types carried by tokens / rules ───────────────── */
%union {
    int   ival;   /* integer value (literals, sizes)   */
    char *str;    /* identifier name or type keyword   */
}

/* ── Token declarations ──────────────────────────────────── */
%token TOK_INT TOK_FLOAT TOK_CHAR
%token TOK_ASSIGN
%token TOK_LBRACKET TOK_RBRACKET
%token TOK_LBRACE   TOK_RBRACE
%token TOK_LPAREN   TOK_RPAREN
%token TOK_COMMA    TOK_SEMICOLON
%token TOK_PLUS TOK_MINUS TOK_MULTIPLY TOK_DIVIDE

%token <str>  TOK_ID
%token <ival> TOK_INT_LITERAL

/* ── Non-terminal types ──────────────────────────────────── */
%type <str>  type
%type <ival> expression term factor

/* ── Operator precedence (lowest to highest) ─────────────── */
%left  TOK_PLUS TOK_MINUS
%left  TOK_MULTIPLY TOK_DIVIDE

%%

/* ══════════════════════════════════════════════════════════
   Grammar Rules
   ══════════════════════════════════════════════════════════ */

/* Top-level: a program is a list of statements */
program
    : statement_list
        {
            printf("\nParsing complete.\n");
            print_symtab();
            if (error_count == 0)
                printf("\n  Semantic analysis: OK -- no errors found.\n\nResult: ACCEPTED\n");
            else
                printf("\n  Semantic analysis: FAILED -- %d error(s) found.\n\nResult: REJECTED\n",
                       error_count);
        }
    ;

/* One or more statements (left-recursive — preferred by YACC) */
statement_list
    : statement_list statement
    | statement
    ;

/* A statement is either a declaration or an assignment, followed by ; */
statement
    : declaration TOK_SEMICOLON
    | assignment  TOK_SEMICOLON
    | error TOK_SEMICOLON
        { yyerrok; fprintf(stderr, "  Recovered from syntax error at line %d\n", line_num); }
    ;

/* ── Declarations ────────────────────────────────────────── */

/* int arr[5];   or   int arr[3] = {1, 2, 3}; */
declaration
    : type TOK_ID TOK_LBRACKET TOK_INT_LITERAL TOK_RBRACKET
        {
            /* Rule 6: size must be positive */
            if ($4 <= 0) {
                fprintf(stderr,
                    "Semantic Error [line %d]: array '%s' size must be positive\n",
                    line_num, $2);
                error_count++;
            } else {
                sym_insert($2, $1, $4, line_num);
            }
            free($2);
        }

    | type TOK_ID TOK_LBRACKET TOK_INT_LITERAL TOK_RBRACKET
        TOK_ASSIGN TOK_LBRACE initializer_list TOK_RBRACE
        {
            if ($4 <= 0) {
                fprintf(stderr,
                    "Semantic Error [line %d]: array '%s' size must be positive\n",
                    line_num, $2);
                error_count++;
            } else {
                sym_insert($2, $1, $4, line_num);
            }
            free($2);
        }
    ;

/* Type keyword — carry the string name for the symbol table */
type
    : TOK_INT   { $$ = "int";   }
    | TOK_FLOAT { $$ = "float"; }
    | TOK_CHAR  { $$ = "char";  }
    ;

/* Comma-separated list of initializer expressions */
initializer_list
    : expression
    | initializer_list TOK_COMMA expression
    ;

/* ── Assignments ─────────────────────────────────────────── */

/* arr = expr;   or   arr[expr] = expr; */
assignment
    : TOK_ID TOK_ASSIGN expression
        {
            /* Rule 1: must be declared */
            if (!sym_lookup($1)) {
                fprintf(stderr,
                    "Semantic Error [line %d]: undeclared array '%s'\n",
                    line_num, $1);
                error_count++;
            }
            free($1);
        }

    | TOK_ID TOK_LBRACKET expression TOK_RBRACKET TOK_ASSIGN expression
        {
            /* Rule 1: must be declared */
            Sym *s = sym_lookup($1);
            if (!s) {
                fprintf(stderr,
                    "Semantic Error [line %d]: undeclared array '%s'\n",
                    line_num, $1);
                error_count++;
            }
            free($1);
        }
    ;

/* ── Expressions ─────────────────────────────────────────── */

/* Left-recursive expression rules — natural for YACC */
expression
    : expression TOK_PLUS  term   { $$ = $1 + $3; }
    | expression TOK_MINUS term   { $$ = $1 - $3; }
    | term                        { $$ = $1; }
    ;

term
    : term TOK_MULTIPLY factor    { $$ = $1 * $3; }
    | term TOK_DIVIDE   factor    { $$ = $3 != 0 ? $1 / $3 : 0; }
    | factor                      { $$ = $1; }
    ;

factor
    : TOK_INT_LITERAL             { $$ = $1; }
    | TOK_ID
        {
            /* Rule 1: identifier used in expression must be declared */
            if (!sym_lookup($1)) {
                fprintf(stderr,
                    "Semantic Error [line %d]: undeclared array '%s'\n",
                    line_num, $1);
                error_count++;
            }
            free($1);
            $$ = 0;
        }
    | TOK_ID TOK_LBRACKET expression TOK_RBRACKET
        {
            Sym *s = sym_lookup($1);
            if (!s) {
                fprintf(stderr,
                    "Semantic Error [line %d]: undeclared array '%s'\n",
                    line_num, $1);
                error_count++;
            }
            free($1);
            $$ = 0;
        }
    | TOK_LPAREN expression TOK_RPAREN   { $$ = $2; }
    ;

%%

/* ── Error handler ───────────────────────────────────────── */
void yyerror(const char *msg) {
    fprintf(stderr, "Syntax Error [line %d]: %s\n", line_num, msg);
    error_count++;
}

/* ── Main ────────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    printf("Array Declaration Compiler (Lex/YACC version)\n");
    printf("----------------------------------------------\n");

    if (argc >= 2) {
        FILE *fp = fopen(argv[1], "r");
        if (!fp) {
            fprintf(stderr, "Error: cannot open '%s'\n", argv[1]);
            return 1;
        }
        extern FILE *yyin;
        yyin = fp;
    }

    yyparse();
    return (error_count > 0) ? 1 : 0;
}
