# Array Declaration Language Grammar

`TYPE` is one of `int`, `float`, or `char`.

```ebnf
program             -> statement* EOF ;
statement           -> arrayDeclaration | assignment ;

arrayDeclaration    -> TYPE IDENTIFIER "[" expression? "]"
                       ("=" initializer)? ";" ;
initializer         -> "{" (expression ("," expression)* ","?)? "}" ;
assignment          -> lvalue "=" expression ";" ;
lvalue              -> IDENTIFIER ("[" expression "]")? ;

expression          -> addition ;
addition            -> multiplication (("+" | "-") multiplication)* ;
multiplication      -> unary (("*" | "/" | "%") unary)* ;
unary               -> ("+" | "-") unary | primary ;
primary             -> INTEGER | FLOAT | CHARACTER
                     | IDENTIFIER ("[" expression "]")?
                     | "(" expression ")" ;
```

Additional parser rule: unsized declarations require an initializer.

## Semantic checks

The semantic analyzer enforces:
- array sizes must be positive integer constant expressions
- declarations are unique within a program
- identifiers and arrays must be declared before use
- array indexes must be integer expressions
- literal integer indexes must be in bounds
- initializer count cannot exceed declared size
- assignment and initializer values must follow conversion rules:
  - allowed: exact match, `int->float`, `char->float`, `char->int`, `int->char`
  - rejected: `float->int`, `float->char`
