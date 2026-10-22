# Array Declaration Mini Compiler — Lexer and Parser in C

This project implements the lexical-analysis and syntax-analysis portions of a
mini compiler front end using **standard C (C11)**. It recognizes array
declarations, brace initializers, array indexing, and element assignments, then
builds an Abstract Syntax Tree (AST).

## Supported syntax

```c
int marks[5];
int primes[4] = {2, 3, 5, 7};
float readings[] = {1.5, 2.25, 3.0};
char grades[3] = {'A', 'B', 'C'};
marks[2] = 10;
marks[1 + 2] = primes[0] * 2;
x = marks[2];
```

The lexer recognizes:

- types: `int`, `float`, `char`
- identifiers, integer/float literals, and character literals
- brackets, braces, parentheses, commas, semicolons, assignment, and arithmetic
  operators
- whitespace, `//` comments, and `/* ... */` comments

Invalid characters and malformed character literals/comments produce lexical
errors with line and column information. The recursive-descent parser reports
missing delimiters and other invalid token sequences in the same format.

## Build and run

A C11 compiler is the only dependency. With GCC/MinGW:

```powershell
gcc -std=c11 -Wall -Wextra -Werror -pedantic -Iinclude src/compiler.c src/token.c src/lexer.c src/ast.c src/parser.c src/main.c -o mini_compiler.exe
./mini_compiler.exe examples/demo.arr
./mini_compiler.exe --tokens -c "int arr[5];"
./mini_compiler.exe --ast -c "arr[2] = 10;"
```

With neither output flag, the CLI prints both the token stream and JSON AST.

## Test

```powershell
gcc -std=c11 -Wall -Wextra -Werror -pedantic -Iinclude src/compiler.c src/token.c src/lexer.c src/ast.c src/parser.c tests/test_compiler.c -o compiler_tests.exe
./compiler_tests.exe
```

The tests cover valid declarations/initializers/assignments and invalid lexical
and syntax cases. See [GRAMMAR.md](GRAMMAR.md) for the formal grammar and a
sample derivation.

## Project structure

```text
include/          public C headers for tokens, lexer, parser, and AST
grammar/          original group CFG documentation and derivations
src/
  lexer.c         lexical analyzer
  parser.c        recursive-descent syntax analyzer
  ast.c           AST ownership and JSON output
  main.c          command-line demonstration
tests/            C test harness with valid and invalid cases
examples/         sample array-language source program
lexar.c           preserved standalone lexer from the base `nibin` branch
```

`src/lexer.c` is the modular lexer used by the parser and command-line program.
The earlier standalone `lexar.c` remains in the repository so the original
Member 2 contribution and its Git history are preserved.

Semantic rules—such as whether an array was declared, whether its size is
positive, whether initializers match the element type, and whether indices are
integers/in range—are intentionally left for the semantic-analysis stage.
