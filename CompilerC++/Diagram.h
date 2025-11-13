#pragma once
#include "Scanner.h"
#include "Defines.h"
#include "DataType.h"
#include "Tree.h"
#include <string>
#include <vector>
#include <stack>

using std::string;

class Diagram {
private:
    Scanner* sc;
    std::vector<int> pushTok;
    std::vector<string> pushLex;
    int curTok;
    string curLex;
    DATA_TYPE currentDeclType;

    // Стек для вычисления выражений
    std::stack<SemNode> evalStack;

    int nextToken();
    int peekToken();
    void pushBack(int tok, const string& lex);
    void lexError();
    void synError(const string& msg);
    void semError(const string& msg);
	void interpError(const string& msg);

    // Синтаксические процедуры с интерпретацией
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
    bool CaseStmt(long long switchVal, bool& anyMatched);
    bool DefaultStmt(bool anyMatched);
    void Name();

    // Выражения с интерпретацией
    DATA_TYPE Expr();
    DATA_TYPE Rel();
    DATA_TYPE Shift();
    DATA_TYPE Add();
    DATA_TYPE Mul();
    DATA_TYPE Prim();
    void Call();
    void ArgListOpt(std::vector<SemNode>& args);
    void Const();

    // Вспомогательные методы интерпретации
    void pushValue(const SemNode& node);
    SemNode popValue();
    SemNode evaluateConstant(const string& value, DATA_TYPE type);
    void executeAssignment(const string& varName, DATA_TYPE exprType, int line, int col);

public:
    Diagram(Scanner* scanner);
    void ParseProgram(bool isInterp = true, bool isDebug = false);
};