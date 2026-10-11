#pragma once
// Shared helpers for the test suites.

#include "test_framework.h"
#include "../CompilerC++/Scanner.h"
#include "../CompilerC++/Tree.h"
#include "../CompilerC++/Diagram.h"

#include <sstream>
#include <string>

// Reset the global tree state and create a fresh root scope.
// Needed because the symbol tree keeps static state shared between runs.
inline void resetSymbolTable() {
    Tree::reset();

    SemNode* root = new SemNode();
    root->id = "root";
    root->DataType = TYPE_SCOPE;
    Tree::Root = new Tree(root, nullptr);
    Tree::Cur = Tree::Root;
}

// Result of running a program through the interpreter.
struct RunResult {
    bool threw = false;   // whether an exception escaped the compiler
    std::string output;   // everything written to stdout and stderr
};

// Compile (and optionally interpret) the given source, capturing all output.
inline RunResult runProgram(const std::string& source, bool interp = true, bool debug = true) {
    RunResult result;

    Tree::reset();
    Scanner sc;
    sc.loadFromString(source);

    std::ostringstream captured;
    std::streambuf* oldOut = std::cout.rdbuf(captured.rdbuf());
    std::streambuf* oldErr = std::cerr.rdbuf(captured.rdbuf());

    try {
        Diagram dg(&sc);
        dg.ParseProgram(interp, debug);
    }
    catch (...) {
        result.threw = true;
    }

    std::cout.rdbuf(oldOut);
    std::cerr.rdbuf(oldErr);

    result.output = captured.str();
    return result;
}

// True if `haystack` contains `needle`.
inline bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}
