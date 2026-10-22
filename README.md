# Array Declaration Mini Compiler

This repository now uses one coherent implementation: the modular
`Program`/`Statement`/`Expression` AST with `TokenArray` and `TOKEN_*` token
types from `include/`.

## Supported language

```c
int a[3] = {1, 2, 3,};
float f[] = {1.5, -2.0, +3};
char letters[3] = {'A', 'B', '\n'};
a[1 + 1] = (a[0] * 2) % 3;
```

Features:
- `int`, `float`, `char` array declarations
- integer, float, and character literals
- `//` and `/* ... */` comments
- array element assignments and scalar assignments (`id = expression;`)
- operators `+`, `-`, `*`, `/`, `%` with precedence
- unary `+` and unary `-`
- parentheses
- brace initializers, including optional trailing comma (`{1,2,3,}`) and empty initializer (`{}`)

Unsized declarations (`type name[]`) must include an initializer.

## Semantic rules

The semantic pass rejects:
- non-positive array sizes
- duplicate declarations
- undeclared identifiers/arrays
- non-integer indexes
- out-of-range literal indexes
- initializer lists larger than declared size
- incompatible assignment/initializer types

Conversion rules:
- exact type matches are allowed
- `int -> float`, `char -> float`, `char -> int`, and `int -> char` are allowed
- `float -> int` and `float -> char` are rejected

## Build and test

```sh
make
make test
```

`make test` compiles and runs unit tests, then checks `tests/inputs` for expected
accept/reject behavior using compiler exit status.
