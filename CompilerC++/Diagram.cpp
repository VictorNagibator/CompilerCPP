#include "Diagram.h"
#include "Tree.h"
#include <iostream>
#include <exception>
#include <stdexcept>

Diagram::Diagram(Scanner* scanner) : sc(scanner), curTok(0), curLex(), currentDeclType(TYPE_INT) {
    pushTok.clear();
    pushLex.clear();
}

void Diagram::synError(const string& msg) {
    auto lc = sc->getLineCol();
    std::cerr << "Syntax error: " << msg;
    if (!curLex.empty()) std::cerr << " (near '" << curLex << "')";
    std::cerr << std::endl << "(line " << lc.first << ":" << lc.second << ")" << std::endl;
    throw std::runtime_error("Syntax error");
}

void Diagram::lexError() {
    auto lc = sc->getLineCol();
    std::cerr << "Lexical error: unknown token '" << curLex << "'";
    std::cerr << std::endl << "(line " << lc.first << ":" << lc.second << ")" << std::endl;
    throw std::runtime_error("Lexical error");
}

void Diagram::interpError(const string& msg) {
    auto lc = sc->getLineCol();
    Tree::interpError(msg, curLex, lc.first, lc.second);
}

void Diagram::semError(const string& msg) {
    auto lc = sc->getLineCol();
    Tree::semError(msg, curLex, lc.first, lc.second);
}

int Diagram::nextToken() {
    if (!pushTok.empty()) {
        int t = pushTok.back();
        pushTok.pop_back();
        string lx = pushLex.back();
        pushLex.pop_back();
        curTok = t;
        curLex = lx;
        if (curTok == T_ERR) lexError();
        return curTok;
    }

    string lex;
    curTok = sc->getNextLex(lex);
    curLex = lex;

    if (curTok == T_ERR) lexError();
    return curTok;
}

int Diagram::peekToken() {
    int t = nextToken();
    pushBack(t, curLex);
    return t;
}

void Diagram::pushBack(int tok, const string& lex) {
    pushTok.push_back(tok);
    pushLex.push_back(lex);
}

// Interpreter helpers
void Diagram::pushValue(const SemNode& node) {
    evalStack.push(node);
}

SemNode Diagram::popValue() {
    if (evalStack.empty()) {
        semError("internal error: evaluation stack is empty");
    }
    SemNode node = evalStack.top();
    evalStack.pop();
    return node;
}

SemNode Diagram::evaluateConstant(const string& value, DATA_TYPE type) {
    SemNode result;
    result.DataType = type;
    result.hasValue = true;

    try {
        if (type == TYPE_BOOL) {
            result.Value.v_bool = (value == "true");
        }
        else {
            string valueToParse = value;
            bool isNegative = false;

            if (!valueToParse.empty() && valueToParse[0] == '-') {
                isNegative = true;
                valueToParse = valueToParse.substr(1);
            }

            long long val = std::stoll(valueToParse, nullptr, 0);
            if (isNegative) val = -val;

            if (type == TYPE_SHORT_INT) {
                result.Value.v_int16 = static_cast<int16_t>(val);
            }
            else if (type == TYPE_INT) {
                result.Value.v_int32 = static_cast<int32_t>(val);
            }
            else if (type == TYPE_LONG_INT) {
                result.Value.v_int64 = static_cast<int64_t>(val);
            }
        }
    }
    catch (const std::exception& e) {
        semError("invalid constant format: " + string(e.what()));
    }

    return result;
}

void Diagram::executeAssignment(const string& varName, DATA_TYPE exprType, int line, int col) {
    SemNode value = popValue();
    Tree* varNode = Tree::Cur->semGetVar(varName, line, col);
    DATA_TYPE varType = varNode->n->DataType;

    bool varIsInt = (varType == TYPE_INT || varType == TYPE_SHORT_INT || varType == TYPE_LONG_INT);
    bool exprIsInt = (exprType == TYPE_INT || exprType == TYPE_SHORT_INT || exprType == TYPE_LONG_INT);

    if (!((varIsInt && exprIsInt) || (varType == TYPE_BOOL && exprType == TYPE_BOOL))) {
        semError("type mismatch in assignment to '" + varName + "'");
    }

    Tree::setVarValue(varName, value, line, col);
}

// Entry point
void Diagram::ParseProgram(bool isInterp, bool isDebug) {
    SemNode* rootNode = new SemNode();
    rootNode->id = "<global scope>";
    rootNode->DataType = TYPE_SCOPE;
    rootNode->line = 0;
    rootNode->col = 0;
    Tree* rootTree = new Tree(rootNode, nullptr);
    Tree::setCur(rootTree);

    if (isInterp) Tree::enableInterpretation();
    else          Tree::disableInterpretation();

    if (isDebug) Tree::enableDebug();
    else         Tree::disableDebug();

    Program();

    int t = nextToken();
    if (t != T_END) synError("unexpected text after the end of the program");

    if (!isInterp) {
        rootTree->print();
    }
}

// Program -> TopDecl*
void Diagram::Program() {
    int t = peekToken();
    while (t == KW_VOID || t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL) {
        TopDecl();
        t = peekToken();
    }
}

// TopDecl -> Function | VarDecl
void Diagram::TopDecl() {
    int t = peekToken();
    if (t == KW_VOID) {
        Function();
    }
    else {
        VarDecl();
    }
}

// Function -> 'void' IDENT '(' ParamListOpt ')' Block
void Diagram::Function() {
    int t = nextToken();
    if (t != KW_VOID) synError("expected 'void' in a function definition");

    t = nextToken();
    if (t != IDENT && t != KW_MAIN) synError("expected a function name (identifier)");
    string funcName = curLex;
    auto pos = sc->getLineCol();

    Tree* funcNode = Tree::Cur->semInclude(funcName, TYPE_FUNCT, pos.first, pos.second);
    Tree* savedCur = Tree::Cur;

    if (funcNode && funcNode->Left) {
        Tree::setCur(funcNode->Left);
    }
    else {
        Tree::Cur->semEnterBlock(pos.first, pos.second);
    }

    t = nextToken();
    if (t != LPAREN) synError("expected '(' after the function name");

    std::vector<DATA_TYPE> paramTypes;
    int paramCount = 0;
    t = peekToken();
    if (t != RPAREN) {
        for (;;) {
            t = nextToken();
            if (!(t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL))
                synError("expected a parameter type");

            DATA_TYPE ptype = TYPE_INT;
            if (t == KW_SHORT) ptype = TYPE_SHORT_INT;
            else if (t == KW_LONG) ptype = TYPE_LONG_INT;
            else if (t == KW_BOOL) ptype = TYPE_BOOL;

            t = nextToken();
            if (t != IDENT) synError("expected a parameter name");
            string paramName = curLex;
            auto ppos = sc->getLineCol();

            Tree* paramNode = Tree::Cur->semInclude(paramName, ptype, ppos.first, ppos.second);
            if (paramNode && paramNode->n) {
                paramNode->n->hasValue = true; // parameters are considered initialized
            }
            paramTypes.push_back(ptype);
            paramCount++;

            t = peekToken();
            if (t == COMMA) {
                nextToken();
                continue;
            }
            else {
                break;
            }
        }
    }

    t = nextToken();
    if (t != RPAREN) synError("expected ')' after the parameter list");

    funcNode->semSetParam(funcNode, paramCount);
    funcNode->semSetParamTypes(funcNode, paramTypes);

    Tree::setCurrentFunction(funcNode);

    // Only `main` (without parameters) is executed while being parsed.
    bool wasInterpretationEnabled = Tree::isInterpretationEnabled();
    bool isEntryPoint = (funcName == "main" && paramCount == 0);
    if (!isEntryPoint) {
        Tree::disableInterpretation();
    }

    if (funcNode && funcNode->n) {
        funcNode->n->bodyStartPos = sc->getPos();
    }

    Block();

    if (funcNode && funcNode->n) {
        funcNode->n->bodyEndPos = sc->getPos();
    }

    Tree::setCurrentFunction(nullptr);

    if (!isEntryPoint) {
        if (wasInterpretationEnabled) Tree::enableInterpretation();
        else                          Tree::disableInterpretation();
    }

    Tree::setCur(savedCur);
}

// VarDecl -> Type IdInitList ';'
void Diagram::VarDecl() {
    int t = nextToken();
    if (!(t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL))
        synError("expected a type in a variable declaration");

    if (t == KW_SHORT) currentDeclType = TYPE_SHORT_INT;
    else if (t == KW_LONG) currentDeclType = TYPE_LONG_INT;
    else if (t == KW_BOOL) currentDeclType = TYPE_BOOL;
    else currentDeclType = TYPE_INT;

    IdInitList();

    t = nextToken();
    if (t != SEMI) synError("expected ';' at the end of a variable declaration");
}

// IdInitList -> IdInit (',' IdInit)*
void Diagram::IdInitList() {
    IdInit();
    int t = peekToken();
    while (t == COMMA) {
        nextToken();
        IdInit();
        t = peekToken();
    }
}

// IdInit -> IDENT ['=' Expr]
void Diagram::IdInit() {
    int t = nextToken();
    if (t != IDENT) synError("expected an identifier in the declaration list");
    string varName = curLex;
    auto pos = sc->getLineCol();

    Tree* varNode = Tree::Cur->semInclude(varName, currentDeclType, pos.first, pos.second);

    t = peekToken();
    if (t == ASSIGN) {
        nextToken();

        DATA_TYPE exprType = Expr();
        SemNode value = popValue();

        DATA_TYPE varType = varNode->n->DataType;
        bool varIsInt = (varType == TYPE_INT || varType == TYPE_SHORT_INT || varType == TYPE_LONG_INT);
        bool exprIsInt = (exprType == TYPE_INT || exprType == TYPE_SHORT_INT || exprType == TYPE_LONG_INT);

        if (!((varIsInt && exprIsInt) || (varType == TYPE_BOOL && exprType == TYPE_BOOL))) {
            semError("type mismatch in the initializer of variable '" + varName + "'");
        }

        Tree::setVarValue(varName, value, pos.first, pos.second);
    }
}

// Block -> '{' BlockItems '}'
void Diagram::Block() {
    int t = nextToken();
    if (t != LBRACE) synError("expected '{' to open a block");

    size_t posAfterLBrace = sc->getPos();

    auto lc = sc->getLineCol();
    Tree::Cur->semEnterBlock(lc.first, lc.second);

    // Record the body start of the enclosing function only once.
    Tree* curFunc = Tree::getCurrentFunction();
    if (curFunc && curFunc->n) {
        if (curFunc->n->bodyStartPos == 0) curFunc->n->bodyStartPos = posAfterLBrace;
    }

    BlockItems();

    t = nextToken();
    if (t != RBRACE) synError("expected '}' to close a block");

    if (curFunc && curFunc->n) {
        curFunc->n->bodyEndPos = sc->getPos();
    }

    Tree::Cur->semExitBlock();
}

// BlockItems -> ( VarDecl | Stmt )*
void Diagram::BlockItems() {
    int t = peekToken();
    while (t != RBRACE && t != T_END) {
        if (t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL) {
            VarDecl();
        }
        else {
            Stmt();
        }
        t = peekToken();
    }
}

// Stmt -> ';' | Block | Assign ';' | SwitchStmt | CallStmt
void Diagram::Stmt() {
    int t = peekToken();

    if (t == SEMI) {
        nextToken(); // empty statement
        return;
    }

    if (t == LBRACE) {
        Block();
        return;
    }

    if (t == KW_SWITCH) {
        SwitchStmt();
        return;
    }

    if (t == IDENT) {
        int tokIdent = nextToken();
        string savedName = curLex;
        int t2 = peekToken();

        pushBack(tokIdent, savedName);

        if (t2 == ASSIGN) {
            Assign();
            t = nextToken();
            if (t != SEMI) synError("expected ';' after an assignment");
            return;
        }
        else if (t2 == LPAREN) {
            CallStmt();
            return;
        }
        else {
            synError("expected '=' (assignment) or '(' (function call) after an identifier");
        }
    }

    synError("unknown statement form");
}

// CallStmt -> Call ';'
void Diagram::CallStmt() {
    Call();

    int t = nextToken();
    if (t != SEMI) synError("expected ';' after a function call");
}

// Assign -> IDENT '=' Expr
void Diagram::Assign() {
    int t = nextToken();
    if (t != IDENT) synError("expected an identifier in an assignment");
    string name = curLex;
    auto lc = sc->getLineCol();

    Tree* leftNode = Tree::Cur->semGetVar(name, lc.first, lc.second);
    if (leftNode->n->DataType == TYPE_FUNCT) {
        semError("cannot assign to function '" + name + "'");
    }

    t = nextToken();
    if (t != ASSIGN) synError("expected '=' in an assignment");

    DATA_TYPE rtype = Expr();
    DATA_TYPE ltype = leftNode->n->DataType;

    bool lIsInt = (ltype == TYPE_INT || ltype == TYPE_SHORT_INT || ltype == TYPE_LONG_INT);
    bool rIsInt = (rtype == TYPE_INT || rtype == TYPE_SHORT_INT || rtype == TYPE_LONG_INT);

    if (!((lIsInt && rIsInt) || (ltype == TYPE_BOOL && rtype == TYPE_BOOL))) {
        semError("type mismatch in assignment to '" + name + "'");
    }

    executeAssignment(name, rtype, lc.first, lc.second);
}

// SwitchStmt -> 'switch' '(' Expr ')' '{' CaseStmt* DefaultStmt? '}'
// Cases fall through to the next clause until a 'break' is reached, matching C.
void Diagram::SwitchStmt() {
    int t = nextToken();
    if (t != KW_SWITCH) synError("expected 'switch'");

    t = nextToken();
    if (t != LPAREN) synError("expected '(' after 'switch'");

    DATA_TYPE stype = Expr();
    if (!(stype == TYPE_INT || stype == TYPE_SHORT_INT || stype == TYPE_LONG_INT)) {
        semError("the switch expression must have an integer type (int/short/long)");
    }

    SemNode sVal = popValue();
    if (!sVal.hasValue) interpError("switch expression is uninitialized");
    long long switchVal = 0;
    if (sVal.DataType == TYPE_SHORT_INT) switchVal = sVal.Value.v_int16;
    else if (sVal.DataType == TYPE_INT) switchVal = sVal.Value.v_int32;
    else if (sVal.DataType == TYPE_LONG_INT) switchVal = sVal.Value.v_int64;

    t = nextToken();
    if (t != RPAREN) synError("expected ')' after the switch expression");

    t = nextToken();
    if (t != LBRACE) synError("expected '{' to open the switch body");

    bool executing = false;   // statements are currently being executed
    bool switchEnded = false; // a break has terminated the switch

    int pk = peekToken();
    while (pk == KW_CASE) {
        CaseStmt(switchVal, executing, switchEnded);
        pk = peekToken();
    }

    if (pk == KW_DEFAULT) {
        DefaultStmt(executing, switchEnded);
    }

    t = nextToken();
    if (t != RBRACE) synError("expected '}' to close the switch body");
}

// CaseStmt -> 'case' Const ':' Stmt*
void Diagram::CaseStmt(long long switchVal, bool& executing, bool& switchEnded) {
    int t = nextToken();
    if (t != KW_CASE) synError("expected 'case'");

    t = nextToken();
    if (!(t == DEC_CONST || t == HEX_CONST)) {
        semError("a case label must be a numeric constant");
    }

    long long caseVal = 0;
    try {
        caseVal = std::stoll(curLex, nullptr, 0);
    }
    catch (...) {
        semError("invalid constant in a case label");
    }

    t = nextToken();
    if (t != COLON) synError("expected ':' after a case label");

    if (!executing && !switchEnded && caseVal == switchVal) {
        executing = true;
    }

    for (;;) {
        int p = peekToken();
        if (p == KW_CASE || p == KW_DEFAULT || p == RBRACE || p == T_END) break;

        if (p == KW_BREAK) {
            nextToken(); // break
            int semi = nextToken();
            if (semi != SEMI) synError("expected ';' after 'break'");
            if (executing && !switchEnded) switchEnded = true;
            continue;
        }

        if (Tree::isInterpretationEnabled() && executing && !switchEnded) {
            Stmt();
        }
        else {
            bool saved = Tree::isInterpretationEnabled();
            if (saved) Tree::disableInterpretation();
            Stmt();
            if (saved) Tree::enableInterpretation();
        }
    }
}

// DefaultStmt -> 'default' ':' Stmt*
void Diagram::DefaultStmt(bool& executing, bool& switchEnded) {
    int t = nextToken();
    if (t != KW_DEFAULT) synError("expected 'default'");

    t = nextToken();
    if (t != COLON) synError("expected ':' after 'default'");

    if (!executing && !switchEnded) {
        executing = true;
    }

    for (;;) {
        int p = peekToken();
        if (p == KW_CASE || p == RBRACE || p == T_END) break;

        if (p == KW_BREAK) {
            nextToken();
            int semi = nextToken();
            if (semi != SEMI) synError("expected ';' after 'break'");
            if (executing && !switchEnded) switchEnded = true;
            continue;
        }

        if (Tree::isInterpretationEnabled() && executing && !switchEnded) {
            Stmt();
        }
        else {
            bool saved = Tree::isInterpretationEnabled();
            if (saved) Tree::disableInterpretation();
            Stmt();
            if (saved) Tree::enableInterpretation();
        }
    }
}

// Call -> IDENT '(' ArgListOpt ')'
void Diagram::Call() {
    int t = nextToken();
    if (t != IDENT) synError("expected a function name in a call");
    string fname = curLex;
    auto lc = sc->getLineCol();

    Tree* fnode = Tree::Cur->semGetFunct(fname, lc.first, lc.second);

    t = nextToken();
    if (t != LPAREN) synError("expected '(' after the function name");

    std::vector<SemNode> args;
    ArgListOpt(args);

    t = nextToken();
    if (t != RPAREN) synError("expected ')' after the argument list");

    std::vector<DATA_TYPE> argTypes;
    for (const auto& arg : args) argTypes.push_back(arg.DataType);
    fnode->semControlParamTypes(fnode, argTypes, lc.first, lc.second);

    Tree::printFunctionCall(fname, args, lc.first, lc.second);

    if (!Tree::isInterpretationEnabled()) return;

    Tree::enterFunctionCall(fname, lc.first, lc.second);

    Tree* funcScope = fnode->Left;
    if (!funcScope) interpError("function body is missing in call to '" + fname + "'");

    // Save the parser state.
    size_t savedPos = sc->getPos();
    auto savedPushTok = pushTok;
    auto savedPushLex = pushLex;
    int savedCurTok = curTok;
    string savedCurLex = curLex;
    Tree* savedCurTree = Tree::getCur();
    Tree* savedCurrentFunction = Tree::getCurrentFunction();

    // Create a fresh scope for the call, linked to the function.
    SemNode* tmpScopeNode = new SemNode();
    tmpScopeNode->id = ""; tmpScopeNode->DataType = TYPE_SCOPE;
    tmpScopeNode->Param = 0; tmpScopeNode->line = lc.first; tmpScopeNode->col = lc.second;
    Tree* tmpScope = new Tree(tmpScopeNode, fnode);

    // Copy the parameter descriptors (names/types) into the temporary scope.
    Tree* origP = funcScope->Left;
    for (size_t i = 0; origP && i < args.size(); origP = origP->Right, ++i) {
        if (!origP->n) continue;
        SemNode* pcopy = new SemNode(*origP->n);
        pcopy->hasValue = false;
        pcopy->line = origP->n->line;
        pcopy->col = origP->n->col;
        tmpScope->setLeft(pcopy);
    }

    // Bind the actual argument values to the copied parameters.
    Tree* copyP = tmpScope->Left;
    for (size_t i = 0; copyP && i < args.size(); copyP = copyP->Right, ++i) {
        if (!copyP->n) continue;

        SemNode converted = Tree::castToType(args[i], copyP->n->DataType, lc.first, lc.second);

        copyP->n->hasValue = converted.hasValue;
        copyP->n->Value = converted.Value;

        if (args[i].DataType != copyP->n->DataType && Tree::isDebugEnabled()) {
            Tree::printTypeConversionWarning(args[i].DataType, copyP->n->DataType,
                "parameter passing", copyP->n->id + " in " + fname + "()", lc.first, lc.second);
        }
    }

    // Execute the body in the temporary scope. Collect any exception so that
    // the parser state is always restored and the temporary scope is freed.
    Tree::setCur(tmpScope);
    Tree::setCurrentFunction(fnode);

    size_t bodyStart = (fnode->n) ? fnode->n->bodyStartPos : 0;
    sc->setPos(bodyStart);

    std::exception_ptr error;
    try {
        Block();
    }
    catch (...) {
        error = std::current_exception();
    }

    Tree::exitFunctionCall();

    // Restore the parser state.
    sc->setPos(savedPos);
    pushTok = savedPushTok;
    pushLex = savedPushLex;
    curTok = savedCurTok;
    curLex = savedCurLex;
    Tree::setCur(savedCurTree);
    Tree::setCurrentFunction(savedCurrentFunction);

    delete tmpScope;

    if (error) {
        std::rethrow_exception(error);
    }
}

// ArgListOpt -> [Expr (',' Expr)*]
void Diagram::ArgListOpt(std::vector<SemNode>& args) {
    int t = peekToken();
    if (t != RPAREN) {
        Expr();
        args.push_back(popValue());

        t = peekToken();
        while (t == COMMA) {
            nextToken();
            Expr();
            args.push_back(popValue());
            t = peekToken();
        }
    }
}

// Expr -> Rel ( ('==' | '!=') Rel )*
DATA_TYPE Diagram::Expr() {
    DATA_TYPE left = Rel();

    int t = peekToken();
    while (t == EQ || t == NEQ) {
        string op = (t == EQ) ? "==" : "!=";
        nextToken();
        DATA_TYPE right = Rel();

        SemNode rightVal = popValue();
        SemNode leftVal = popValue();

        bool leftIsInt = (left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT);
        bool rightIsInt = (right == TYPE_INT || right == TYPE_SHORT_INT || right == TYPE_LONG_INT);

        if ((leftIsInt && rightIsInt) || (left == TYPE_BOOL && right == TYPE_BOOL)) {
            SemNode result = Tree::executeComparisonOp(leftVal, rightVal, op, sc->getLineCol().first, sc->getLineCol().second);
            pushValue(result);
            left = TYPE_BOOL;
        }
        else {
            semError("operands of '=='/'!=' must have the same type");
        }

        t = peekToken();
    }

    return left;
}

// Rel -> Shift ( ('<' | '<=' | '>' | '>=') Shift )*
DATA_TYPE Diagram::Rel() {
    DATA_TYPE left = Shift();
    int t = peekToken();

    while (t == LT || t == LE || t == GT || t == GE) {
        string op;
        switch (t) {
        case LT: op = "<"; break;
        case LE: op = "<="; break;
        case GT: op = ">"; break;
        case GE: op = ">="; break;
        }
        nextToken();
        DATA_TYPE right = Shift();

        SemNode rightVal = popValue();
        SemNode leftVal = popValue();

        bool lInt = (left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT);
        bool rInt = (right == TYPE_INT || right == TYPE_SHORT_INT || right == TYPE_LONG_INT);

        if (!(lInt && rInt)) {
            semError("operands of '<', '<=', '>', '>=' must be integers (int/short/long)");
        }

        SemNode result = Tree::executeComparisonOp(leftVal, rightVal, op, sc->getLineCol().first, sc->getLineCol().second);
        pushValue(result);
        left = TYPE_BOOL;
        t = peekToken();
    }
    return left;
}

// Shift -> Add ( ('<<' | '>>') Add )*
DATA_TYPE Diagram::Shift() {
    DATA_TYPE left = Add();
    int t = peekToken();

    while (t == SHL || t == SHR) {
        string op = (t == SHL) ? "<<" : ">>";
        nextToken();
        DATA_TYPE right = Add();

        SemNode rightVal = popValue();
        SemNode leftVal = popValue();

        bool lInt = (left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT);
        bool rInt = (right == TYPE_INT || right == TYPE_SHORT_INT || right == TYPE_LONG_INT);

        if (!(lInt && rInt)) {
            semError("operands of shifts must be integers (int/short/long)");
        }

        SemNode result = Tree::executeShiftOp(leftVal, rightVal, op, sc->getLineCol().first, sc->getLineCol().second);
        pushValue(result);
        left = result.DataType;
        t = peekToken();
    }
    return left;
}

// Add -> Mul ( ('+' | '-') Mul )*
DATA_TYPE Diagram::Add() {
    DATA_TYPE left = Mul();
    int t = peekToken();

    while (t == PLUS || t == MINUS) {
        string op = (t == PLUS) ? "+" : "-";
        nextToken();
        DATA_TYPE right = Mul();

        SemNode rightVal = popValue();
        SemNode leftVal = popValue();

        bool lInt = (left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT);
        bool rInt = (right == TYPE_INT || right == TYPE_SHORT_INT || right == TYPE_LONG_INT);

        if (!(lInt && rInt)) {
            semError("operands of '+'/'-' must be integers (int/short/long)");
        }

        SemNode result = Tree::executeArithmeticOp(leftVal, rightVal, op, sc->getLineCol().first, sc->getLineCol().second);
        pushValue(result);
        left = result.DataType;
        t = peekToken();
    }
    return left;
}

// Mul -> Prim ( ('*' | '/' | '%') Prim )*
DATA_TYPE Diagram::Mul() {
    DATA_TYPE left = Prim();
    int t = peekToken();

    while (t == MULT || t == DIV || t == MOD) {
        string op;
        switch (t) {
        case MULT: op = "*"; break;
        case DIV: op = "/"; break;
        case MOD: op = "%"; break;
        }
        nextToken();
        DATA_TYPE right = Prim();

        SemNode rightVal = popValue();
        SemNode leftVal = popValue();

        bool lInt = (left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT);
        bool rInt = (right == TYPE_INT || right == TYPE_SHORT_INT || right == TYPE_LONG_INT);

        if (!(lInt && rInt)) {
            semError("operands of '*', '/', '%' must be integers (int/short/long)");
        }

        SemNode result = Tree::executeArithmeticOp(leftVal, rightVal, op, sc->getLineCol().first, sc->getLineCol().second);
        pushValue(result);
        left = result.DataType;
        t = peekToken();
    }
    return left;
}

// Prim -> ['+'|'-'] ( IDENT | Const | '(' Expr ')' )
DATA_TYPE Diagram::Prim() {
    int t = nextToken();

    // Unary plus/minus
    if (t == PLUS || t == MINUS) {
        bool negative = (t == MINUS);
        int t2 = peekToken();

        if (t2 == DEC_CONST || t2 == HEX_CONST) {
            // Negative/positive literal: fold the sign into the constant and
            // promote the type to the smallest type that can hold the value.
            nextToken(); // consume the constant
            string valueStr = curLex;

            DATA_TYPE constType = TYPE_SHORT_INT;
            try {
                long long val = std::stoll(valueStr, nullptr, 0);
                if (val > 32767 || val < -32768) constType = TYPE_INT;
                if (val > 2147483647LL || val < -2147483648LL) constType = TYPE_LONG_INT;
            }
            catch (...) {
                constType = TYPE_INT;
            }

            string signedValue = negative ? ("-" + valueStr) : valueStr;
            SemNode constNode = evaluateConstant(signedValue, constType);
            pushValue(constNode);
            return constType;
        }

        DATA_TYPE operandType = Prim();
        if (!(operandType == TYPE_INT || operandType == TYPE_SHORT_INT || operandType == TYPE_LONG_INT)) {
            semError("unary '+'/'-' can only be applied to integer types");
        }

        if (!negative) {
            return operandType; // unary plus does not change the value
        }

        SemNode operand = popValue();
        SemNode minusOne;
        minusOne.DataType = operandType;
        minusOne.hasValue = true;
        switch (operandType) {
        case TYPE_SHORT_INT: minusOne.Value.v_int16 = -1; break;
        case TYPE_INT:       minusOne.Value.v_int32 = -1; break;
        case TYPE_LONG_INT:  minusOne.Value.v_int64 = -1; break;
        default: break;
        }

        SemNode result = Tree::executeArithmeticOp(operand, minusOne, "*",
            sc->getLineCol().first, sc->getLineCol().second);
        pushValue(result);
        return result.DataType;
    }

    if (t == DEC_CONST || t == HEX_CONST) {
        DATA_TYPE constType = TYPE_SHORT_INT;
        try {
            long long val = std::stoll(curLex, nullptr, 0);
            if (val > 32767 || val < -32768) constType = TYPE_INT;
            if (val > 2147483647LL || val < -2147483648LL) constType = TYPE_LONG_INT;
        }
        catch (...) {
            constType = TYPE_INT;
        }

        SemNode constNode = evaluateConstant(curLex, constType);
        pushValue(constNode);
        return constType;
    }

    if (t == KW_TRUE || t == KW_FALSE) {
        SemNode boolNode;
        boolNode.DataType = TYPE_BOOL;
        boolNode.hasValue = true;
        boolNode.Value.v_bool = (t == KW_TRUE);
        pushValue(boolNode);
        return TYPE_BOOL;
    }

    if (t == LPAREN) {
        DATA_TYPE dt = Expr();
        t = nextToken();
        if (t != RPAREN) synError("expected ')' after the expression");
        return dt;
    }

    if (t == IDENT) {
        string name = curLex;
        int t2 = peekToken();

        if (t2 == LPAREN) {
            semError("a function call cannot be used inside an expression: functions return void");
        }
        else {
            auto lc = sc->getLineCol();
            Tree* v = Tree::Cur->semGetVar(name, lc.first, lc.second);

            if (!v->n->hasValue) {
                interpError("use of uninitialized variable '" + name + "'");
            }

            SemNode value;
            value.DataType = v->n->DataType;
            value.hasValue = true;
            value.Value = v->n->Value;
            pushValue(value);

            return v->n->DataType;
        }
    }

    synError("expected a primary expression (identifier, constant or parenthesized expression)");
    return TYPE_INT;
}
