#include "test_helpers.h"

TEST(Interpreter_SimpleArithmetic) {
    RunResult r = runProgram(R"(
void main() {
    int x = 10;
    int y = 20;
    int z = 0;
    z = x + y;
}
)");
    CHECK(!r.threw);
    CHECK(contains(r.output, "Assignment: x = 10 (int)"));
    CHECK(contains(r.output, "Assignment: z = 30 (int)"));
}

TEST(Interpreter_FunctionCallAndGlobalState) {
    RunResult r = runProgram(R"(
int a = 0;
void store(int v) {
    a = v;
}
void main() {
    store(7);
}
)");
    CHECK(!r.threw);
    CHECK(contains(r.output, "Function call: store(7"));
    CHECK(contains(r.output, "Assignment: a = 7 (int)"));
}

TEST(Interpreter_MultipleFunctionCalls) {
    RunResult r = runProgram(R"(
int a = 0;
void add(int n) { a = a + n; }
void mul(int n) { a = a * n; }
void main() {
    add(3);
    mul(4);
}
)");
    CHECK(!r.threw);
    CHECK(contains(r.output, "Assignment: a = 3 (int)"));
    CHECK(contains(r.output, "Assignment: a = 12 (int)"));
}

TEST(Interpreter_SwitchFallThrough) {
    RunResult r = runProgram(R"(
void main() {
    int x = 1;
    int r = 0;
    switch (x) {
        case 1:
            r = 10;
        case 2:
            r = r + 5;
            break;
        case 3:
            r = 100;
        default:
            r = 999;
    }
}
)");
    CHECK(!r.threw);
    CHECK(contains(r.output, "Assignment: r = 10 (int)"));
    CHECK(contains(r.output, "Assignment: r = 15 (int)"));
    CHECK(!contains(r.output, "r = 100"));
    CHECK(!contains(r.output, "r = 999"));
}

TEST(Interpreter_SwitchDefault) {
    RunResult r = runProgram(R"(
void main() {
    int x = 7;
    int r = 0;
    switch (x) {
        case 1:
            r = 1;
            break;
        default:
            r = 42;
            break;
    }
}
)");
    CHECK(!r.threw);
    CHECK(contains(r.output, "Assignment: r = 42 (int)"));
}

TEST(Interpreter_SwitchBreakStopsFallThrough) {
    RunResult r = runProgram(R"(
void main() {
    int x = 1;
    int r = 0;
    switch (x) {
        case 1:
            r = 10;
            break;
        case 2:
            r = 20;
            break;
    }
}
)");
    CHECK(!r.threw);
    CHECK(contains(r.output, "Assignment: r = 10 (int)"));
    CHECK(!contains(r.output, "r = 20"));
}

TEST(Interpreter_Recursion) {
    RunResult r = runProgram(R"(
long acc = 0;
void count(int n) {
    switch (n) {
        case 0:
            break;
        default:
            acc = acc + n;
            count(n - 1);
            break;
    }
}
void main() {
    count(5);
}
)");
    CHECK(!r.threw);
    CHECK(contains(r.output, "Assignment: acc = 15 (long)"));
}

TEST(Interpreter_RecursionLimitIsEnforced) {
    RunResult r = runProgram(R"(
void loop(int n) {
    loop(n);
}
void main() {
    loop(1);
}
)");
    CHECK(r.threw);
}

TEST(Interpreter_TypeMismatchThrows) {
    RunResult r = runProgram(R"(
void main() {
    int x = 1;
    bool b = true;
    x = b;
}
)");
    CHECK(r.threw);
}

TEST(Interpreter_ParameterTypeMismatchThrows) {
    RunResult r = runProgram(R"(
void f(int a) {
}
void main() {
    bool b = true;
    f(b);
}
)");
    CHECK(r.threw);
}

TEST(Interpreter_DivisionByZeroThrows) {
    RunResult r = runProgram(R"(
void main() {
    int x = 10;
    int y = 0;
    int z = x / y;
}
)");
    CHECK(r.threw);
}

TEST(Interpreter_UninitializedVariableThrows) {
    RunResult r = runProgram(R"(
void main() {
    int x;
    int y = x;
}
)");
    CHECK(r.threw);
}

TEST(Interpreter_AnalysisOnlyPrintsTree) {
    RunResult r = runProgram(R"(
int g = 1;
void main() {
}
)", /*interp=*/false, /*debug=*/false);
    CHECK(!r.threw);
    CHECK(contains(r.output, "<global scope>"));
    CHECK(contains(r.output, "main"));
    CHECK(contains(r.output, "g"));
}

TEST(Interpreter_NegativeLiteral) {
    RunResult r = runProgram(R"(
void main() {
    int x = -5;
    int y = 3;
    int z = x + y;
}
)");
    CHECK(!r.threw);
    CHECK(contains(r.output, "Assignment: x = -5 (int)"));
    CHECK(contains(r.output, "Assignment: z = -2 (int)"));
}

TEST(Interpreter_UnaryMinusOnVariable) {
    RunResult r = runProgram(R"(
void main() {
    int x = 5;
    int y = -x;
}
)");
    CHECK(!r.threw);
    CHECK(contains(r.output, "Assignment: y = -5 (int)"));
}

TEST(Interpreter_BooleanValues) {
    RunResult r = runProgram(R"(
void main() {
    bool b = true;
    bool c = false;
}
)");
    CHECK(!r.threw);
    CHECK(contains(r.output, "Assignment: b = true (bool)"));
    CHECK(contains(r.output, "Assignment: c = false (bool)"));
}

TEST(Interpreter_DebugOutputCanBeDisabled) {
    RunResult r = runProgram(R"(
void main() {
    int x = 1;
    int y = 2;
    int z = x + y;
}
)", /*interp=*/true, /*debug=*/false);
    CHECK(!r.threw);
    CHECK(r.output.empty());
}
