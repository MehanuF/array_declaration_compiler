# Array Declaration Language Grammar

The parser uses the following unambiguous, left-recursion-free grammar. `TYPE`
is one of `int`, `float`, or `char`.

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

The parser adds one syntax constraint: if an array size is omitted, an
initializer must be present. Type compatibility, declaration lookup, duplicate
names, initializer length, and index type/range belong to semantic analysis and
are intentionally not performed by this component.

## Example derivation

For `int arr[5];`:

```text
program
=> statement EOF
=> arrayDeclaration EOF
=> TYPE IDENTIFIER "[" expression "]" ";" EOF
=> int arr "[" INTEGER "]" ";" EOF
=> int arr[5]; EOF
```
