# Array Declaration Compiler

A compiler front end for a simplified C-like language supporting single-dimensional array declarations, initialization, indexing, element assignments, and arithmetic expressions.

## Features

- Array declarations: `int arr[5];`, `float values[10];`, `char letters[4];`
- Array initialization: `int marks[3] = {10, 20, 30};`
- Array element assignment: `arr[2] = 50;`
- Arithmetic expressions: `arr[1 + 2] = 8;`  (supports `+`, `-`, `*`, `/`)
- Integer literals only (no floating-point or character literals in source code)
- Semantic checks: undeclared arrays, bounds checking, type checking, initializer count

## Build & Run

```bash
make                              # compile
./array_compiler tests/inputs/v4.c  # run on a file
make test                         # run all tests with exit code verification
make clean                        # remove build artifacts
```

## How It Works

```
Source Code  →  Lexer  →  Parser  →  AST  →  Semantic Analyzer  →  OK / Error
```

The compiler has three phases:

1. **Lexer** (`src/lexer.c`) — tokenizes source into a token stream
2. **Parser** (`src/parser.c`) — recursive-descent LL(1) parser builds an AST
3. **Semantic Analyzer** (`src/semantic.c`) — walks the AST and checks rules

## Grammar (LL(1), left-recursion-free)

```
Program         → StatementList
StatementList   → Statement StatementList | ε
Statement       → Declaration ; | Assignment ;
Declaration     → Type ID [ INT_LITERAL ] InitializerOpt
Type            → int | float | char
InitializerOpt  → = { InitializerList } | ε
InitializerList → Expression InitializerTail
InitializerTail → , Expression InitializerTail | ε
Assignment      → LValue = Expression
LValue          → ID | ID [ Expression ]
Expression      → Term ExpressionTail
ExpressionTail  → + Term ExpressionTail | - Term ExpressionTail | ε
Term            → Factor TermTail
TermTail        → * Factor TermTail | / Factor TermTail | ε
Factor          → ID | INT_LITERAL | ( Expression ) | ID [ Expression ]
```

## Semantic Rules

| # | Rule |
|---|------|
| 1 | Array must be declared before use |
| 2 | Array index must be of integer type |
| 3 | Literal index must be within declared bounds |
| 4 | Assigned values must be type-compatible |
| 5 | Initializer count must not exceed array size |
| 6 | Array size must be a positive integer |

Redeclaration and type mismatches are treated as errors.

## Repository Structure

```
array_declaration_compiler/
├── grammar/
│   ├── array_grammar.txt          # Context-Free Grammar
│   ├── grammar_documentation.md   # Grammar design & transformations
│   └── sample_derivations.md      # Leftmost derivation examples
├── src/
│   ├── lexer.h / lexer.c          # Lexical Analyzer
│   ├── ast.h / ast.c              # AST node definitions + tree printer
│   ├── parser.h / parser.c        # Recursive-Descent Parser
│   ├── semantic.h / semantic.c    # Semantic Analyzer
│   └── main.c                     # Main driver
├── tests/
│   ├── grammar_test_cases.md      # Test case specification
│   └── inputs/                    # Test input files
│       ├── v1.c … v10.c           # Valid (expect ACCEPTED)
│       ├── i1.c … i7.c            # Invalid syntax (expect REJECTED)
│       └── s1.c … s4.c            # Semantic errors (expect REJECTED)
├── Makefile
└── README.md
```

## Test Results

| ID  | Input | Expected | Verified |
|-----|-------|----------|----------|
| V1  | `int arr[5];` | ACCEPTED | ✅ |
| V2  | `float values[10];` | ACCEPTED | ✅ |
| V3  | `char letters[4];` | ACCEPTED | ✅ |
| V4  | `int marks[3] = {10, 20, 30};` | ACCEPTED | ✅ |
| V5  | `int a[2] = {1, 2};` | ACCEPTED | ✅ |
| V6  | `arr[2] = 50;` (after declaration) | ACCEPTED | ✅ |
| V7  | `x[0] = arr[2];` (after declarations) | ACCEPTED | ✅ |
| V8  | `arr[1 + 2] = 8;` (after declaration) | ACCEPTED | ✅ |
| V9  | `int a[4] = {1 + 2, 5, 8, 10};` | ACCEPTED | ✅ |
| V10 | `int arr[3] = {1, 2, 3};` | ACCEPTED | ✅ |
| I1  | `int [5];` | REJECTED (missing ID) | ✅ |
| I2  | `int arr[5]` | REJECTED (missing `;`) | ✅ |
| I3  | `int arr[];` | REJECTED (missing size) | ✅ |
| I4  | `int arr[5 = 2;` | REJECTED (missing `]`) | ✅ |
| I5  | `int arr[3] = {1, 2, 3;` | REJECTED (missing `}`) | ✅ |
| I6  | `arr[2] = ;` | REJECTED (missing expr) | ✅ |
| I7  | `int arr[3] = {, 1};` | REJECTED (missing expr) | ✅ |
| S1  | `unknown[1] = 5;` | REJECTED (undeclared) | ✅ |
| S2  | `arr[5] = 10;` (size 5) | REJECTED (out of bounds) | ✅ |
| S3  | `int arr[3] = {1,2,3,4};` | REJECTED (excess init) | ✅ |
| S4  | `int arr[0];` | REJECTED (non-positive size) | ✅ |

## Limitations

- Single-dimensional arrays only
- Integer literals only (no float/char literals in source)
- No comments, no unary operators, no `%` operator
- No pointers, functions, or control flow

## Team Contributions

| Member | Role | Files |
|--------|------|-------|
| Member 1 | Grammar Designer | `grammar/` |
| Member 2 | Lexical Analyzer | `src/lexer.c`, `src/lexer.h` |
| Member 3 | Parser + AST | `src/parser.c`, `src/ast.c` |
| Member 4 | Semantic Analyzer | `src/semantic.c` |
