
# Array Declaration Compiler

## Project Overview

The Array Declaration Compiler is a mini compiler front end for a simplified C-like language. It supports array declarations, initialization, indexing, element assignments, and arithmetic expressions.

## Project Objectives

- Define a Context-Free Grammar (CFG) for array-related statements.
- Identify valid and invalid syntax.
- Support grammar transformations required for parsing.
- Document sample derivations.
- Prepare test cases for syntax and semantic validation.

## Supported Features

- Array declarations using `int`, `float`, and `char`.
- Optional array initialization.
- Array element access and assignment.
- Arithmetic expressions using `+`, `-`, `*`, and `/`.
- Basic semantic checks, including declaration, index type, bounds, and type compatibility.

## Repository Structure

```text
array_declaration_compiler/
├── grammar/
│   ├── array_grammar.txt
│   └── grammar_documentation.md
├── tests/
│   └── grammar_test_cases.md
├── README.md
└── .gitignore
```

## Grammar Designer — Member 1

Responsibilities:

- Design and document the CFG.
- Define grammar rules for declarations, initialization, indexing, and assignments.
- Explain grammar transformations.
- Provide sample derivations and test cases.

## Current Scope

This repository currently contains grammar documentation and test-case specifications. The complete compiler front end will also require implementation of the lexer, parser, and semantic analyzer.

## Team Contributions

Each member contributes to the assigned component of the compiler front end. Individual contributions are documented for the project presentation and viva.
