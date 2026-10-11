#pragma once
#include "Scanner.h"
#include "Defines.h"
#include "DataType.h"
#include "Tree.h"
#include <string>
#include <vector>
#include <stack>

using std::string;

// Recursive-descent parser and tree-walking interpreter for the language.
class Diagram {
private:
    Scanner* sc;
    std::vector<int> pushTok;      // pushback buffer for tokens
    std::vector<string> pushLex;   // pushback buffer for token texts
    int curTok;
    string curLex;
    DATA_TYPE currentDeclType;

    // Stack used to evaluate expressions while parsing
    std::stack<SemNode> evalStack;

    int nextToken();
    int peekToken();
    void pushBack(int tok, const string& lex);
    void lexError();
    void synError(const string& msg);
    void semError(const string& msg);
    void interpError(const string& msg);

    // Grammar productions
    void Program();
    void TopDecl();
    void Function();
    void VarDecl();
    void IdInitList();
    void IdInit();
    void Block();
    void BlockItems();
    void Stmt();
    void Assign();
    void CallStmt();
    void SwitchStmt();
    void CaseStmt(long long switchVal, bool& executing, bool& switchEnded);
    void DefaultStmt(bool& executing, bool& switchEnded);

    // Expressions (each returns the static type of the parsed expression)
    DATA_TYPE Expr();
    DATA_TYPE Rel();
    DATA_TYPE Shift();
    DATA_TYPE Add();
    DATA_TYPE Mul();
    DATA_TYPE Prim();
    void Call();
    void ArgListOpt(std::vector<SemNode>& args);

    // Interpreter helpers
    void pushValue(const SemNode& node);
    SemNode popValue();
    SemNode evaluateConstant(const string& value, DATA_TYPE type);
    void executeAssignment(const string& varName, DATA_TYPE exprType, int line, int col);

public:
    Diagram(Scanner* scanner);
    // Parse the whole program. When isInterp is true the code is executed;
    // when isDebug is true runtime diagnostics are printed.
    void ParseProgram(bool isInterp = true, bool isDebug = false);
};
