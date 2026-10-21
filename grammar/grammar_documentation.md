
# Array Declaration Compiler — Grammar Documentation

## 1. Introduction

This project develops a compiler front end for a simplified C-like language supporting single-dimensional array declarations, initialization, indexing, element assignments, and arithmetic expressions.

The Context-Free Grammar (CFG) defines the syntax accepted by the parser.

## 2. Scope

The grammar supports:
- Array declarations using int, float, and char.
- Optional array initialization.
- Array element access and assignments.
- Arithmetic expressions using +, -, *, and /.
- Multiple statements in one input.

## 3. Sample Input

```c
int arr[5];
int marks[3] = {10, 20, 30};
arr[2] = 50;
x = arr[2];
```

## 4. Sample Derivation

Input:

```c
int arr[5];
```

Derivation using the CFG:

```text
Program
=> StatementList
=> Statement StatementList
=> Declaration ; StatementList
=> Type ID [ INT_LITERAL ] InitializerOpt ;
=> int arr [ 5 ] epsilon ;
=> int arr [ 5 ] ;
```

Here, `epsilon` represents an empty production. It means that the array has no initializer.

## 5. Array Initialization

Example:

```c
int marks[3] = {10, 20, 30};
```

The declaration specifies the type, identifier, size, and initializer list. The initializer list contains comma-separated expressions inside braces.

## 6. Array Element Assignment

Example:

```c
arr[2] = 50;
```

The left-hand side identifies an array element. The right-hand side provides the expression assigned to that element.

## 7. Grammar Design and Transformations

### Expression grammar

```text
Expression -> Term ExpressionTail

ExpressionTail -> + Term ExpressionTail
                | - Term ExpressionTail
                | epsilon

Term -> Factor TermTail

TermTail -> * Factor TermTail
          | / Factor TermTail
          | epsilon
```

The grammar separates expressions and terms to give multiplication and division higher precedence than addition and subtraction.

The expression-tail and term-tail productions avoid left recursion in arithmetic expressions, making this structure suitable for predictive parsing.

No additional left factoring is required for these expression productions.

## 8. Semantic Checks

The CFG checks syntax. The semantic analyzer must additionally check:

1. Whether an array is declared before use.
2. Whether an index has an integer type.
3. Whether the index is within the declared array bounds.
4. Whether assigned values are compatible with the array element type.
5. Whether the initializer count is compatible with the declared array size.

## 9. Limitations

The current grammar supports a simplified C-like syntax for single-dimensional arrays. Multidimensional arrays, pointers, functions, and advanced C features are outside the current scope.

## 10. Conclusion

The CFG provides the syntactic foundation for the Array Declaration Compiler. It defines the structure of declarations, initialization, indexing, assignments, and arithmetic expressions. The parser and semantic analyzer can use this grammar as the basis for validating input programs.

