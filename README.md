# Simple Translator / Interpreter for a C-like Language

This project is a **simple translator** (compiler / interpreter) for a small C-like programming language. It performs lexical analysis, recursive-descent parsing, semantic analysis (symbol tables, type checking), and can **interpret** the source program directly, with optional debug output. The implementation is written in C++ and is designed for educational purposes.

## Features

* **Lexical analysis** – recognizes keywords, identifiers, integer constants (decimal/hex), operators, and comments (`//` and `/* */`).
* **Recursive-descent parser** – implements the grammar shown below.
* **Semantic analysis** – builds a syntax tree with symbol tables, checks for duplicate declarations, type compatibility, and function parameter counts.
* **Interpretation** – executes the program by re-parsing function bodies with argument substitution.

  * Supports functions (only `void` type), local blocks, variable assignments, and `switch` statements.
  * Handles recursion with a configurable depth limit (default 50).
  * Performs implicit type conversions (with optional warnings).
* **Debug output** – when enabled, prints detailed information about assignments, function calls, arithmetic operations, and type conversion warnings.
* **Type system** – `int`, `short`, `long`, `bool`. Integer constants automatically promote to the smallest type that can hold their value.

## Building the Project

The project consists of several `.cpp` and `.h` files. You can compile it with any C++11 (or later) compiler.

**Example using g++:**

```bash
g++ -std=c++11 main.cpp Scanner.cpp Diagram.cpp Tree.cpp -o translator
```

**Windows note:** The `main()` function calls `SetConsoleCP(1251)` and `SetConsoleOutputCP(1251)` for correct Russian console output. If you do not need Russian support, you can remove those lines.

## Usage

```
translator [input_file]
```

If no input file is given, it defaults to `input.txt` in the current directory.

The program first performs lexical, syntactic, and semantic analysis.

* If **interpretation is enabled** (default), it then executes the program and prints runtime debug information (if debug mode is on).
* If interpretation is disabled, it only prints the syntax tree.

You can control interpretation and debug flags by modifying the call to `dg.ParseProgram(isInterp, isDebug)` in `main()`.

## Language Syntax

The language is a small subset of C. Below is an informal grammar:

```
Program       -> TopDecl*
TopDecl       -> Function | VarDecl
Function      -> 'void' IDENT '(' ParamListOpt ')' Block
ParamListOpt  -> [ Param (',' Param)* ]
Param         -> Type IDENT
Type          -> 'int' | 'short' | 'long' | 'bool'

VarDecl       -> Type IdInitList ';'
IdInitList    -> IdInit (',' IdInit)*
IdInit        -> IDENT [ '=' Expr ]

Block         -> '{' BlockItems '}'
BlockItems    -> ( VarDecl | Stmt )*

Stmt          -> ';'
              | Block
              | Assign ';'
              | SwitchStmt
              | CallStmt

Assign        -> IDENT '=' Expr
CallStmt      -> Call ';'
Call          -> IDENT '(' ArgListOpt ')'
ArgListOpt    -> [ Expr (',' Expr)* ]

SwitchStmt    -> 'switch' '(' Expr ')' '{' CaseStmt* DefaultStmt? '}'
CaseStmt      -> 'case' Const ':' Stmt*
DefaultStmt   -> 'default' ':' Stmt*

Expr          -> ['+'|'-'] Rel ( ('==' | '!=') Rel )*
Rel           -> Shift ( ('<' | '<=' | '>' | '>=') Shift )*
Shift         -> Add ( ('<<' | '>>') Add )*
Add           -> Mul ( ('+' | '-') Mul )*
Mul           -> Prim ( ('*' | '/' | '%') Prim )*
Prim          -> IDENT
              | Const
              | '(' Expr ')'

Const         -> DEC_CONST | HEX_CONST | 'true' | 'false'
```

### Notes

* All functions must return `void` (no `return` statement).
* Variables must be declared before use (block scope).
* `switch` expressions must be of integer type; `case` labels must be integer constants.
* `break` inside a `switch` exits the switch construct.
* Function calls are statements (cannot be used inside expressions).
* Recursion is allowed.
* No loops (`for`, `while`) – but recursion can be used instead.

## Interpreter Behavior

* **Function calls** – When a function is called, the interpreter:

  1. Saves the current parser state.
  2. Creates a temporary scope for the function’s parameters.
  3. Copies the function body’s source position (recorded during parsing) and re-parses the body in the new scope.
  4. Restores the previous state after execution.
* **Recursion** – limited to 50 nested calls to avoid infinite loops.
* **Type conversions** – Implicit conversions between integer types are allowed. If a value is out of range for the target type, a warning is printed and the value is truncated.
* **Uninitialized variables** – Using a variable before assignment causes an interpretation error.

## Debug Output

When debug mode is enabled (default `true`), the interpreter prints:

* Variable assignments (with value and type)
* Function calls (with arguments)
* Arithmetic operations (with operands and result)
* Type conversion warnings (e.g., `int` to `short`)

Example:

```
DEBUG: [main] (строка 5:3) Присваивание: x = 42 (int)
DEBUG: [main] (строка 6:5) Вызов функции: foo(10 (int), 20 (int))
DEBUG: [foo] (строка 10:7) Арифметическая операция: 10 (int) + 20 (int) = 30 (int)
```

## Example Program

Input (`input.txt`):

```c
void main() {
    int x = 10;
    int y = 20;
    int z;
    z = x + y;
}
```

Output (with debug enabled):

```
DEBUG: [main] (строка 2:5) Присваивание: x = 10 (int)
DEBUG: [main] (строка 3:5) Присваивание: y = 20 (int)
DEBUG: [main] (строка 5:5) Арифметическая операция: 10 (int) + 20 (int) = 30 (int)
DEBUG: [main] (строка 5:5) Присваивание: z = 30 (int)
```

## Testing

The project includes unit tests written for the **Microsoft CppUnitTestFramework**.
Tests cover:

* Lexical analysis (keywords, identifiers, constants, operators, comments)
* Semantic operations (symbol insertion, duplicate checks, variable lookup)
* Expression evaluation (arithmetic, comparisons, shifts)

To run the tests, open the solution in Visual Studio and build the test project.
