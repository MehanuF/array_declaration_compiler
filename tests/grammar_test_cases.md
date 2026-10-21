# Array Declaration Compiler — Grammar Test Cases

## 1. Purpose

These test cases verify the syntax supported by the array declaration grammar. Semantic checks must be tested separately.

## 2. Valid Syntax Tests

| ID | Input | Expected result |
|---|---|---|
| V1 | `int arr[5];` | Accept |
| V2 | `float values[10];` | Accept |
| V3 | `char letters[4];` | Accept |
| V4 | `int marks[3] = {10, 20, 30};` | Accept |
| V5 | `int a[2] = {1, 2};` | Accept |
| V6 | `arr[2] = 50;` | Accept syntactically |
| V7 | `x = arr[2];` | Accept syntactically |
| V8 | `arr[1 + 2] = 8;` | Accept syntactically |
| V9 | `int a[4] = {1 + 2, 5, 8, 10};` | Accept |
| V10 | `int arr[3] = {1, 2, 3};` | Accept syntax; initializer count is checked semantically |

## 3. Invalid Syntax Tests

| ID | Input | Expected result |
|---|---|---|
| I1 | `int [5];` | Reject: missing identifier |
| I2 | `int arr[5]` | Reject: missing semicolon |
| I3 | `int arr[];` | Reject: missing array size |
| I4 | `int arr[5 = 2;` | Reject: missing closing bracket |
| I5 | `int arr[3] = {1, 2, 3;` | Reject: missing closing brace |
| I6 | `arr[2] = ;` | Reject: missing expression |
| I7 | `int arr[3] = {, 1};` | Reject: missing initializer expression |

## 4. Semantic Test Cases

These inputs are syntactically valid or are intended to test semantic rules. The parser and semantic analyzer should be tested separately.

| ID | Scenario | Expected semantic result |
|---|---|---|
| S1 | Access an undeclared array: `unknown[1] = 5;` | Report undeclared array |
| S2 | Access index 5 in an array declared with size 5 | Report out-of-bounds access |
| S3 | Initialize a size-3 array with four values | Report excess initializers |
| S4 | Assign a value incompatible with the array element type | Report type mismatch, if unsupported by the language |

**Note:** The current grammar does not support decimal literals such as `1.5`. Therefore, a floating-point index test should only be added to semantic tests after the grammar and lexer are extended to recognize floating-point literals.

## 5. Important Note

A grammar parser checks whether the input follows the language syntax. It does not, by itself, determine whether an array was declared, whether an index is within bounds, or whether a value has a compatible type.

The actual compiler implementation must be tested against these cases.
