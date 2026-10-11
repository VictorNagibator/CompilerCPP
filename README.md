# CompilerC++ — a translator/interpreter for a small C-like language

[![CI](https://github.com/VictorNagibator/CompilerCPP/actions/workflows/ci.yml/badge.svg)](https://github.com/VictorNagibator/CompilerCPP/actions/workflows/ci.yml)

A compact **lexer, recursive-descent parser, semantic analyzer and tree-walking
interpreter** for a small C-like programming language, written in portable C++17.
It is meant as an educational project that shows the full pipeline of a simple
compiler: tokenizing, parsing, building a symbol tree, type checking and direct
execution of the source program.

The code is fully cross-platform (Linux, macOS and Windows) and is covered by a
dependency-free unit/integration test suite.

## Features

- **Lexical analysis** — keywords, identifiers, decimal/hexadecimal integer
  literals, `true`/`false`, all operators and delimiters, and both `//` and
  `/* ... */` comments.
- **Recursive-descent parser** — implements the grammar described below and
  reports precise `line:column` diagnostics.
- **Semantic analysis** — builds a scope-aware symbol tree, detects redeclarations,
  checks types and function-argument counts, and rejects undeclared identifiers.
- **Tree-walking interpreter** — executes the program by re-parsing function
  bodies with substituted arguments.
  - Functions (all `void`), local/global variables, assignments, `switch`.
  - Proper C-style `switch` **fall-through** and `break`.
  - Recursion with a configurable depth limit (default **50**).
  - Implicit conversions between integer types with range/truncation warnings.
- **Type system** — `int`, `short`, `long`, `bool`. Integer literals are promoted
  to the smallest type able to hold them.
- **Two output modes** — interpret the program with optional debug tracing, or
  analyze it and print the resulting symbol tree.

## Project layout

```
.
  CMakeLists.txt              Cross-platform build (CMake)
  CompilerC++.sln             Visual Studio solution (Windows)
  LICENSE                     MIT license
  CompilerC++/
    CompilerC++.cpp           Command-line entry point
    Scanner.{h,cpp}           Lexer
    Diagram.{h,cpp}           Parser + interpreter
    Tree.{h,cpp}              Symbol tree / runtime environment
    SemNode.h                 Symbol table entry
    DataType.h, Defines.h     Shared enums and token codes
    input.txt                 Sample program
  CompilerTests/
    CompilerTests.cpp         Test runner
    test_*.cpp                Scanner, semantics, expression and interpreter tests
    test_framework.h          Minimal portable test framework
    test_helpers.h            Shared test helpers
  .github/workflows/ci.yml    Continuous integration (Linux/Windows/macOS)
```

## Requirements

- A C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.16+ (recommended), or Visual Studio 2022

## Building

### CMake (Linux, macOS, Windows)

```bash
cmake -S . -B build
cmake --build build
```

This produces:

- `build/translator` (or `build/translator.exe`) — the compiler/interpreter
- `build/compiler_tests` — the test runner

### Direct compilation with g++/clang++

```bash
g++ -std=c++17 -O2 -o translator \
    CompilerC++/CompilerC++.cpp \
    CompilerC++/Scanner.cpp \
    CompilerC++/Tree.cpp \
    CompilerC++/Diagram.cpp
```

### Visual Studio

Open `CompilerC++.sln` and build. The `CompilerC++` project is the CLI; the
`CompilerTests` project builds the test runner as a console application.

## Usage

```
translator [options] [input_file]
```

| Option          | Description                                             |
|-----------------|---------------------------------------------------------|
| `-h`, `--help`  | Show help and exit                                      |
| `--no-interp`   | Only analyze the program and print the symbol tree      |
| `--debug`       | Enable runtime diagnostics (default)                    |
| `--no-debug`, `-q` | Disable runtime diagnostics                          |

If no input file is given, `input.txt` in the current directory is used.
The process exits with `0` on success and a non-zero code if a lexical,
syntax, semantic or interpretation error occurs.

## Language reference

The language is a small subset of C. All functions return `void`; there are no
loops, but recursion is supported.

### Grammar

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

Expr          -> Rel ( ('==' | '!=') Rel )*
Rel           -> Shift ( ('<' | '<=' | '>' | '>=') Shift )*
Shift         -> Add ( ('<<' | '>>') Add )*
Add           -> Mul ( ('+' | '-') Mul )*
Mul           -> Prim ( ('*' | '/' | '%') Prim )*
Prim          -> ['+'|'-'] ( IDENT | Const | '(' Expr ')' )

Const         -> DEC_CONST | HEX_CONST | 'true' | 'false'
```

### Semantics and notes

- Variables must be declared before use; scopes follow the usual block rules and
  inner declarations may shadow outer ones.
- A function must be defined **before** it is called at runtime (i.e. before the
  statement that calls it is executed).
- `switch` expressions must have an integer type; `case` labels must be integer
  constants. Cases **fall through** to the next clause until a `break` (or the
  end of the switch) is reached, matching C semantics.
- Function calls are statements; a function returning `void` cannot appear inside
  an expression.
- Implicit conversions are allowed between `short`, `int` and `long`. Converting
  to a narrower type truncates the value and emits a warning in debug mode.
  Mixing `bool` and integer types is not allowed.
- Integer literals are automatically promoted to the smallest type that fits
  (`short` → `int` → `long`).
- Recursion is limited to 50 nested calls to protect against infinite recursion.

## Example

`input.txt`:

```c
int a = 2;

void add(int n) {
    a = a + n;
}

void mult(int n) {
    a = a * n;
}

void sub(int n, int m) {
    a = a - (n + m);
}

void div(int n) {
    a = a / n;
}

void main() {
    short n = 2;
    short choice = 2;
    switch(choice - 1) {
        case 0:
            add(n);
            break;
        case 1:
            mult(-1);
        case 2:
            sub(1, - 1);
            break;
            a = 999;
        default:
            div(n);
            break;
    }
}
```

Run it:

```bash
./translator CompilerC++/input.txt
```

Sample (truncated) output:

```
DEBUG: [global scope] (line 1:5) Assignment: a = 2 (int)
DEBUG: [main] (line 20:12) Assignment: n = 2 (short)
DEBUG: [main] (line 22:23) Arithmetic operation: 2 (short) - 1 (short) = 1 (short)
DEBUG: [main] (line 27:18) Function call: mult(-1 (short))
DEBUG: [main] (line 27:18) |--> Entering function body mult
DEBUG: [mult] (line 8:15) Arithmetic operation: 2 (int) * -1 (int) = -2 (int)
DEBUG: [mult] (line 8:8) Assignment: a = -2 (int)
DEBUG: [main] (line 29:17) Function call: sub(1 (short), -1 (short))
DEBUG: [sub] (line 12:20) Arithmetic operation: 1 (int) + -1 (int) = 0 (int)
DEBUG: [sub] (line 12:21) Arithmetic operation: -2 (int) - 0 (int) = -2 (int)
DEBUG: [sub] (line 12:8) Assignment: a = -2 (int)
```

To inspect the symbol tree instead of executing:

```bash
./translator --no-interp --no-debug CompilerC++/input.txt
```

## Testing

The test suite is written with a small, dependency-free framework
(`CompilerTests/test_framework.h`) and runs on every platform.

### With CMake/CTest

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

### Directly

```bash
g++ -std=c++17 -o compiler_tests \
    CompilerTests/CompilerTests.cpp \
    CompilerTests/test_scanner.cpp \
    CompilerTests/test_semantics.cpp \
    CompilerTests/test_expressions.cpp \
    CompilerTests/test_interpreter.cpp \
    CompilerC++/Scanner.cpp CompilerC++/Tree.cpp CompilerC++/Diagram.cpp
./compiler_tests
```

On Windows, build and run the `CompilerTests` project from Visual Studio.

The suite covers:

- **Scanner** — keywords, identifiers, literals, comments, operators, error
  cases, position tracking.
- **Semantics** — insertion/lookup, duplicate detection, scope shadowing,
  parameter checks, constant values.
- **Expressions** — arithmetic, comparisons, shifts, type promotion, casts,
  division-by-zero and uninitialized-value errors.
- **Interpreter** — assignments, function calls, `switch` fall-through/default/
  `break`, recursion and the recursion limit, plus error handling and
  analysis-only tree output.
