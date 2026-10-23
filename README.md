# Array Declaration Compiler

A **compiler front end** for a simplified C-like language, built as part of a Compiler Design course project.
Supports array declarations, initialization, indexing, element assignments, and arithmetic expressions.

**Presentation date:** 23 October 2026

---

## Compilation Stages

```
Source Code
    │
    ▼
┌─────────────────────┐
│  Phase 1 — Lexer    │  Breaks source into tokens (keywords, identifiers, literals, operators)
└─────────────────────┘
    │
    ▼
┌───────────────────────────┐
│  Phase 2 — Syntax Analyzer│  Checks grammar using recursive-descent LL(1) parsing, builds AST
└───────────────────────────┘
    │
    ▼
┌──────────────────────────────┐
│  Phase 3 — Semantic Analyzer │  Checks types, array bounds, declarations, initializer counts
└──────────────────────────────┘
    │
    ▼
  ACCEPTED  /  REJECTED
```

---

## Features

| Feature | Example |
|---------|---------|
| Array declaration | `int arr[5];` |
| Float/char declaration | `float values[10];` &nbsp; `char letters[4];` |
| Array with initializer | `int marks[3] = {10, 20, 30};` |
| Array element assignment | `arr[2] = 50;` |
| Arithmetic in expressions | `arr[1 + 2] = 8;` &nbsp;(`+` `-` `*` `/`) |
| Parenthesised expressions | `arr[(2 + 3) * 4] = 1;` |
| Multiple statements | Multiple lines supported |

---

## How to Run

### Prerequisites

- GCC (any C99-compatible version)
- Python 3 (for the GUI)
- Make

### Step 1 — Build the compiler

```bash
cd array_declaration_compiler
make
```

### Step 2 — Launch the GUI *(recommended for demo)*

```bash
python3 gui.py
```

Type code in the **left panel** → click **▶ Compile** → see colour-coded results in 3 tabs on the right:

- **Phase 1 — Tokens**: full token stream with type, lexeme, and source location
- **Phase 2 — Syntax Analyzer**: parse tree (AST) with node types and values
- **Phase 3 — Semantics**: rule-by-rule check results, ACCEPTED / REJECTED verdict

### Step 3 — Command line (alternative)

```bash
./array_compiler tests/inputs/v4.c          # run on a file
echo "int arr[5];" | ./array_compiler       # run from stdin
```

**Exit codes:** `0` = ACCEPTED &nbsp; `1` = syntax error &nbsp; `2` = semantic error

### Run all tests

```bash
make test       # runs all 21 test cases and checks exit codes automatically
```

### Clean

```bash
make clean
```

---

## Context-Free Grammar (CFG)

The grammar is **LL(1)** — left-recursion-free, suitable for predictive (recursive-descent) parsing.

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

> Left recursion removal example:  
> Original: `Expression → Expression + Term | Term`  
> Transformed: `Expression → Term ExpressionTail` and `ExpressionTail → + Term ExpressionTail | ε`

---

## Semantic Rules

| Rule | Check | Implemented |
|------|-------|-------------|
| 1 | Array must be declared before use | ✅ |
| 2 | Array index must be of integer type | ✅ |
| 3 | Literal index must be within declared bounds | ✅ |
| 4 | Assigned / initializer values must be type-compatible | ✅ |
| 5 | Initializer count must not exceed declared array size | ✅ |
| 6 | Array size must be a positive integer | ✅ |
| + | Duplicate declaration is an error | ✅ |

---

## Symbol Table

The semantic analyzer maintains a **symbol table** (`src/semantic.c`) that records, for each declared array:

- Name (identifier)
- Element type (`int` / `float` / `char`)
- Declared size
- Source line of declaration

This table is consulted for every array access and assignment statement.

---

## Test Cases — 21/21 Passing ✅

### Valid inputs (exit 0 = ACCEPTED)

| ID | Input |
|----|-------|
| V1 | `int arr[5];` |
| V2 | `float values[10];` |
| V3 | `char letters[4];` |
| V4 | `int marks[3] = {10, 20, 30};` |
| V5 | `int a[2] = {1, 2};` |
| V6 | `arr[2] = 50;` (after declaration) |
| V7 | `x[0] = arr[2];` (after both declared) |
| V8 | `arr[1 + 2] = 8;` |
| V9 | `int a[4] = {1 + 2, 5, 8, 10};` |
| V10 | `int arr[3] = {1, 2, 3};` |

### Invalid syntax (exit 1 = REJECTED)

| ID | Input | Error |
|----|-------|-------|
| I1 | `int [5];` | Missing identifier |
| I2 | `int arr[5]` | Missing semicolon |
| I3 | `int arr[];` | Missing array size |
| I4 | `int arr[5 = 2;` | Missing `]` |
| I5 | `int arr[3] = {1, 2, 3;` | Missing `}` |
| I6 | `arr[2] = ;` | Missing expression |
| I7 | `int arr[3] = {, 1};` | Missing initializer expression |

### Semantic errors (exit 2 = REJECTED)

| ID | Input | Rule Violated |
|----|-------|---------------|
| S1 | `unknown[1] = 5;` | Rule 1 — undeclared array |
| S2 | `arr[5] = 10;` on a size-5 array | Rule 3 — out of bounds |
| S3 | `int arr[3] = {1, 2, 3, 4};` | Rule 5 — too many initializers |
| S4 | `int arr[0];` | Rule 6 — non-positive size |

---

## Repository Structure

```
array_declaration_compiler/
├── grammar/
│   ├── array_grammar.txt          # Full Context-Free Grammar
│   ├── grammar_documentation.md   # Grammar design & transformations
│   └── sample_derivations.md      # Leftmost derivation examples
├── src/
│   ├── lexer.h   / lexer.c        # Phase 1 — Lexical Analyzer
│   ├── ast.h     / ast.c          # AST node definitions & tree printer
│   ├── parser.h  / parser.c       # Phase 2 — Syntax Analyzer (recursive-descent)
│   ├── semantic.h / semantic.c    # Phase 3 — Semantic Analyzer + Symbol Table
│   └── main.c                     # Driver: connects all 3 phases
├── tests/
│   ├── grammar_test_cases.md      # Test case specification document
│   └── inputs/
│       ├── v1.c … v10.c           # Valid inputs
│       ├── i1.c … i7.c            # Syntax error inputs
│       └── s1.c … s4.c            # Semantic error inputs
├── gui.py                         # Python GUI (tkinter)
├── Makefile
└── README.md
```

---

## Limitations

- Single-dimensional arrays only
- Integer literals only in source (no `1.5` or `'A'`)
- No comments, unary operators, or `%` in source
- No pointers, functions, or control flow

---

## Team Contributions

| Member | Role | Deliverable |
|--------|------|-------------|
| Member 1 | Grammar Designer | CFG, grammar docs, sample derivations, test spec |
| Member 2 | Lexical Analyzer | `src/lexer.c`, `src/lexer.h` |
| Member 3 | Syntax Analyzer + AST | `src/parser.c`, `src/parser.h`, `src/ast.c`, `src/ast.h` |
| Member 4 | Semantic Analyzer | `src/semantic.c`, `src/semantic.h`, symbol table |
