#include "Diagram.h"
#include "Tree.h"
#include <iostream>

// Конструктор
Diagram::Diagram(Scanner* scanner) : sc(scanner), curTok(0), curLex(), currentDeclType(TYPE_INT) {
    pushTok.clear();
    pushLex.clear();
}

void Diagram::synError(const string& msg) {
    auto lc = sc->getLineCol();
    std::cerr << "Синтаксическая ошибка: " << msg;
    if (!curLex.empty()) std::cerr << " (около '" << curLex << "')";
    std::cerr << std::endl << "(строка " << lc.first << ":" << lc.second << ")" << std::endl;
    std::exit(1);
}

void Diagram::lexError() {
    auto lc = sc->getLineCol();
    std::cerr << "Лексическая ошибка: неизвестная лексема '" << curLex << "'";
    std::cerr << std::endl << "(строка " << lc.first << ":" << lc.second << ")" << std::endl;
    std::exit(1);
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

// Вспомогательные методы для интерпретации
void Diagram::pushValue(const SemNode& node) {
    evalStack.push(node);
}

SemNode Diagram::popValue() {
    if (evalStack.empty()) {
        semError("внутренняя ошибка: стек вычислений пуст");
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
            // Обрабатываем отрицательные числа напрямую
            string valueToParse = value;
            bool isNegative = false;

            if (!valueToParse.empty() && valueToParse[0] == '-') {
                isNegative = true;
                valueToParse = valueToParse.substr(1);
            }

            long long val = std::stoll(valueToParse, nullptr, 0);

            if (isNegative) {
                val = -val;
            }

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
        semError("неверный формат константы: " + string(e.what()));
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
        semError("несоответствие типов при присваивании для '" + varName + "'");
    }

    Tree::setVarValue(varName, value, line, col);
}

// Точка входа
void Diagram::ParseProgram(bool isInterp, bool isDebug) {
    SemNode* rootNode = new SemNode();
    rootNode->id = "<глобальная область видимости>";
    rootNode->DataType = TYPE_SCOPE;
    rootNode->line = 0;
    rootNode->col = 0;
    Tree* rootTree = new Tree(rootNode, nullptr);
    Tree::setCur(rootTree);

    if (isInterp) {
        Tree::enableInterpretation();
    }
    else {
        Tree::disableInterpretation();
    }

    if (isDebug) {
        Tree::enableDebug();
    }
    else {
        Tree::disableDebug();
	}

    Program();

    int t = nextToken();
    if (t != T_END) synError("лишний текст в конце программы");

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
    if (t != KW_VOID) synError("ожидался 'void' в определении функции");

    t = nextToken();
    if (t != IDENT && t != KW_MAIN) synError("ожидалось имя функции (IDENT)");
    string funcName = curLex;
    auto pos = sc->getLineCol();

    Tree* funcNode = Tree::Cur->semInclude(funcName, TYPE_FUNCT, pos.first, pos.second);
    Tree* savedCur = Tree::Cur;

    if (funcNode && funcNode->Right) {
        Tree::setCur(funcNode->Right);
    }
    else {
        Tree::Cur->semEnterBlock(pos.first, pos.second);
    }

    t = nextToken();
    if (t != LPAREN) synError("ожидался '(' после имени функции");

    std::vector<DATA_TYPE> paramTypes;
    int paramCount = 0;
    t = peekToken();
    if (t != RPAREN) {
        for (;;) {
            t = nextToken();
            if (!(t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL))
                synError("ожидался тип параметра");

            DATA_TYPE ptype = TYPE_INT;
            if (t == KW_SHORT) ptype = TYPE_SHORT_INT;
            else if (t == KW_LONG) ptype = TYPE_LONG_INT;
            else if (t == KW_BOOL) ptype = TYPE_BOOL;

            t = nextToken();
            if (t != IDENT) synError("ожидалось имя параметра");
            string paramName = curLex;
            auto ppos = sc->getLineCol();

            Tree* paramNode = Tree::Cur->semInclude(paramName, ptype, ppos.first, ppos.second);
            if (paramNode && paramNode->n) {
                paramNode->n->hasValue = true; // Помечаем параметр как инициализированный
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
    if (t != RPAREN) synError("ожидался ')' после списка параметров");

    funcNode->semSetParam(funcNode, paramCount);
    funcNode->semSetParamTypes(funcNode, paramTypes);

    // Устанавливаем текущую функцию
    Tree::setCurrentFunction(funcNode);

    // Управление флагом интерпретации: выключаем для всех функций кроме main
    bool wasInterpretationEnabled = Tree::isInterpretationEnabled();
    if (funcName != "main" || paramCount != 0) {
        Tree::disableInterpretation();
    }

    if (funcNode && funcNode->n) {
        funcNode->n->bodyStartPos = sc->getPos();
    }

    // Тело функции
    Block();

    if (funcNode && funcNode->n) {
        funcNode->n->bodyEndPos = sc->getPos();
    }

    // Сбрасываем текущую функцию
    Tree::setCurrentFunction(nullptr);

    // Восстанавливаем предыдущее состояние флага интерпретации
    if (funcName != "main" || paramCount != 0) {
        if (wasInterpretationEnabled) {
            Tree::enableInterpretation();
        }
        else {
            Tree::disableInterpretation();
        }
    }

    Tree::setCur(savedCur);
}

// VarDecl -> Type IdInitList ;
void Diagram::VarDecl() {
    int t = nextToken();
    if (!(t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL))
        synError("ожидался тип в объявлении переменных");

    if (t == KW_SHORT) currentDeclType = TYPE_SHORT_INT;
    else if (t == KW_LONG) currentDeclType = TYPE_LONG_INT;
    else if (t == KW_BOOL) currentDeclType = TYPE_BOOL;
    else currentDeclType = TYPE_INT;

    IdInitList();

    t = nextToken();
    if (t != SEMI) synError("ожидался ';' в конце объявления переменных");
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

// IdInit -> IDENT [ = Expr ]
void Diagram::IdInit() {
    int t = nextToken();
    if (t != IDENT) synError("ожидался идентификатор в списке объявлений");
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
            semError("несоответствие типов при инициализации переменной '" + varName + "'");
        }

        Tree::setVarValue(varName, value, pos.first, pos.second);
    }
}

// Block -> '{' BlockItems '}'
void Diagram::Block() {
    int t = nextToken();
    if (t != LBRACE) synError("ожидался '{' для начала блока");

    auto lc = sc->getLineCol();
    Tree::Cur->semEnterBlock(lc.first, lc.second);

    BlockItems();

    t = nextToken();
    if (t != RBRACE) synError("ожидался '}' для конца блока");

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

// Stmt -> ';' | Block | Assign | SwitchStmt | CallStmt
void Diagram::Stmt() {
    int t = peekToken();

    if (t == SEMI) {
        nextToken(); // пустой оператор
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
            if (t != SEMI) synError("ожидался ';' после оператора присваивания");
            return;
        }
        else if (t2 == LPAREN) {
            CallStmt();
            return;
        }
        else {
            synError("ожидалось '=' (присваивание) или '(' (вызов функции) после идентификатора");
        }
    }

    synError("неизвестная форма оператора");
}

// CallStmt -> Call ;
void Diagram::CallStmt() {
    Call();

    int t = nextToken();
    if (t != SEMI) synError("ожидался ';' после вызова функции");
}

// Assign -> IDENT = Expr
void Diagram::Assign() {
    int t = nextToken();
    if (t != IDENT) synError("ожидался идентификатор в присваивании");
    string name = curLex;
    auto lc = sc->getLineCol();

    Tree* leftNode = Tree::Cur->semGetVar(name, lc.first, lc.second);
    if (leftNode->n->DataType == TYPE_FUNCT) {
        semError("нельзя присваивать функции '" + name + "'");
    }

    t = nextToken();
    if (t != ASSIGN) synError("ожидался знак '=' в присваивании");

    DATA_TYPE rtype = Expr();
    DATA_TYPE ltype = leftNode->n->DataType;

    bool lIsInt = (ltype == TYPE_INT || ltype == TYPE_SHORT_INT || ltype == TYPE_LONG_INT);
    bool rIsInt = (rtype == TYPE_INT || rtype == TYPE_SHORT_INT || rtype == TYPE_LONG_INT);

    if (!((lIsInt && rIsInt) || (ltype == TYPE_BOOL && rtype == TYPE_BOOL))) {
        semError("несоответствие типов при присваивании для '" + name + "'");
    }

    // Выполняем присваивание
    executeAssignment(name, rtype, lc.first, lc.second);
}

// SwitchStmt -> 'switch' '(' Expr ')' '{' CaseStmt* DefaultStmt? '}'
void Diagram::SwitchStmt() {
    int t = nextToken();
    if (t != KW_SWITCH) synError("ожидался 'switch'");

    t = nextToken();
    if (t != LPAREN) synError("ожидался '(' после 'switch'");

    DATA_TYPE stype = Expr();
    if (!(stype == TYPE_INT || stype == TYPE_SHORT_INT || stype == TYPE_LONG_INT)) {
        semError("тип выражения в switch должен быть целым (int/short/long)");
    }

    // Получаем числовое значение выражения
    SemNode sVal = popValue();
    long long switchVal = 0;
    if (!sVal.hasValue) interpError("выражение switch неинициализировано");
    if (sVal.DataType == TYPE_SHORT_INT) switchVal = sVal.Value.v_int16;
    else if (sVal.DataType == TYPE_INT) switchVal = sVal.Value.v_int32;
    else if (sVal.DataType == TYPE_LONG_INT) switchVal = sVal.Value.v_int64;

    t = nextToken();
    if (t != RPAREN) synError("ожидался ')' после выражения switch");

    t = nextToken();
    if (t != LBRACE) synError("ожидался '{' для тела switch");

    bool anyMatched = false;   // уже была выполненная совпавшая ветка
    bool endSwitch = false;    // флаг: встретили break, прекращаем исполнение ветвей

    // Обрабатываем все case-ветви; но если endSwitch == true — только "семантика" (неисполнение)
    int pk = peekToken();
    while (pk == KW_CASE) {
        if (!endSwitch) {
            // выполняем / семантически проверяем ветку как раньше; CaseStmt должна вернуть true, если encountered break, и нужно завершить выполнение switch
            if (CaseStmt(switchVal, anyMatched)) {
                endSwitch = true;
            }
        }
        else {
            // уже было break — дальше должны просто разобрать (семантика) все последующие case
            bool wasInterp = Tree::isInterpretationEnabled();
            if (wasInterp) Tree::disableInterpretation();
            // вызываем CaseStmt для синтаксического разбора; она не должна выполнять ветки, т.к. интерпретация выключена
            CaseStmt(switchVal, anyMatched);
            if (wasInterp) Tree::enableInterpretation();
        }
        pk = peekToken();
    }

    // Далее может быть default — если после break, просто разобрать его семантически
    if (peekToken() == KW_DEFAULT) {
        if (!endSwitch) {
            if (DefaultStmt(anyMatched)) {
                endSwitch = true;
            }
        }
        else {
            bool wasInterp = Tree::isInterpretationEnabled();
            if (wasInterp) Tree::disableInterpretation();
            DefaultStmt(anyMatched);
            if (wasInterp) Tree::enableInterpretation();
        }
    }

    // На этом месте ожидаем закрывающую скобку
    t = nextToken();
    if (t != RBRACE) synError("ожидался '}' в конце switch");
}

// CaseStmt -> 'case' Const ':' Stmt*
bool Diagram::CaseStmt(long long switchVal, bool& anyMatched) {
    int t = nextToken();
    if (t != KW_CASE) synError("ожидался 'case'");

    t = nextToken();
    if (!(t == DEC_CONST || t == HEX_CONST)) {
        semError("case принимает только числовую константу");
    }

    // получаем значение case
    long long caseVal = 0;
    try {
        caseVal = std::stoll(curLex, nullptr, 0);
    }
    catch (...) {
        semError("неверная константа в case");
    }

    t = nextToken();
    if (t != COLON) synError("ожидался ':' после case-значения");

    bool matched = (!anyMatched && caseVal == switchVal);
    if (matched) anyMatched = true;

    // Обрабатываем последовательность операторов внутри case
    while (true) {
        int p = peekToken();
        if (p == KW_CASE || p == KW_DEFAULT || p == RBRACE || p == T_END) break;

        if (p == KW_BREAK) {
            nextToken(); // consume break
            int semi = nextToken();
            if (semi != SEMI) synError("ожидался ';' после break");
            // Если мы исполняли этот case (matched==true), то break прекращает весь switch
            return matched;
        }
        else {
            if (Tree::isInterpretationEnabled()) {
                if (matched) {
                    Stmt(); // выполняем операторы в этой ветке
                }
                else {
                    // Не эта ветка — выполняем только семантику: временно выключаем интерпретацию
                    bool saveInterp = Tree::isInterpretationEnabled();
                    if (saveInterp) Tree::disableInterpretation();
                    Stmt();
                    if (saveInterp) Tree::enableInterpretation();
                }
            }
            else {
                // Интерпретация выключена — просто семантика
                Stmt();
            }
        }
    }

    return false;
}

// DefaultStmt -> 'default' ':' Stmt*
bool Diagram::DefaultStmt(bool anyMatched) {
    int t = nextToken();
    if (t != KW_DEFAULT) synError("ожидался 'default'");

    t = nextToken();
    if (t != COLON) synError("ожидался ':' после default");

    bool matched = !anyMatched;

    while (true) {
        int p = peekToken();
        if (p == KW_CASE || p == RBRACE || p == T_END) break;

        if (p == KW_BREAK) {
            nextToken();
            int semi = nextToken();
            if (semi != SEMI) synError("ожидался ';' после break");
            return matched;
        }
        else {
            if (Tree::isInterpretationEnabled()) {
                if (matched) Stmt();
                else {
                    bool saveInterp = Tree::isInterpretationEnabled();
                    if (saveInterp) Tree::disableInterpretation();
                    Stmt();
                    if (saveInterp) Tree::enableInterpretation();
                }
            }
            else {
                Stmt();
            }
        }
    }

    return false;
}

// Call -> IDENT '(' ArgListOpt ')'
void Diagram::Call() {
    int t = nextToken();
    if (t != IDENT) synError("ожидалось имя функции при вызове");
    string fname = curLex;
    auto lc = sc->getLineCol();

    Tree* fnode = Tree::Cur->semGetFunct(fname, lc.first, lc.second);

    t = nextToken();
    if (t != LPAREN) synError("ожидался '(' после имени функции");

    std::vector<SemNode> args;
    ArgListOpt(args);

    t = nextToken();
    if (t != RPAREN) synError("ожидался ')' после списка аргументов");

    // Преобразуем аргументы в типы для проверки
    std::vector<DATA_TYPE> argTypes;
    for (const auto& arg : args) argTypes.push_back(arg.DataType);

    // Семантическая проверка параметров
    fnode->semControlParamTypes(fnode, argTypes, lc.first, lc.second);

    // Печать вызова (семантика/лог)
    Tree::printFunctionCall(fname, args, lc.first, lc.second);

    // Если интерпретация выключена — больше ничего не делаем (только семантика)
    if (!Tree::isInterpretationEnabled()) return;

    // --- Выполнение реального вызова функции ---
    // 1) клонируем область функции (чтобы параметры/локалы были отдельными)
    Tree* funcScope = fnode->Right; // у тебя тело/область функции хранится здесь
    if (!funcScope) {
        interpError("отсутствует тело функции при вызове '" + fname + "'");
    }

    Tree* tmpScope = Tree::cloneSubtree(funcScope);
    if (!tmpScope) {
        interpError("не удалось клонировать область функции '" + fname + "'");
    }

    // 2) сохраняем контекст парсера/сканера
    size_t savedPos = sc->getPos();
    auto savedPushTok = pushTok;
    auto savedPushLex = pushLex;
    int savedCurTok = curTok;
    string savedCurLex = curLex;
    Tree* savedCurTree = Tree::getCur();
    Tree* savedCurrentFunction = Tree::getCurrentFunction();

    // 3) переключаем текущую область на временную копию
    Tree::setCur(tmpScope);
    Tree::setCurrentFunction(fnode);

    // 4) подставляем значения параметров в tmpScope
    Tree* p = tmpScope->Left;
    for (size_t i = 0; i < args.size(); ++i) {
        if (!p) break;
        if (p->n) {
            p->n->hasValue = true;
            p->n->Value = args[i].Value;
            p->n->DataType = args[i].DataType; // хотя тип уже должен быть задан
        }
        p = p->Right;
    }

    // 5) ставим сканер на начало тела функции и исполняем Block()
    size_t bodyStart = fnode->n ? fnode->n->bodyStartPos : 0;
    sc->setPos(bodyStart);

    // Выполняем тело (Block() использует Tree::Cur и интерпретацию включена)
    Block();

    // 6) восстановление контекста
    sc->setPos(savedPos);
    pushTok = savedPushTok;
    pushLex = savedPushLex;
    curTok = savedCurTok;
    curLex = savedCurLex;
    Tree::setCur(savedCurTree);
    Tree::setCurrentFunction(savedCurrentFunction);

    // 7) удаляем временную область
    delete tmpScope;
}

// ArgListOpt -> [Expr (',' Expr)*]
void Diagram::ArgListOpt(std::vector<SemNode>& args) {
    int t = peekToken();
    if (t != RPAREN) {
        DATA_TYPE atype = Expr();
        SemNode arg = popValue();
        args.push_back(arg);

        t = peekToken();
        while (t == COMMA) {
            nextToken();
            atype = Expr();
            arg = popValue();
            args.push_back(arg);
            t = peekToken();
        }
    }
}

// Expr -> ['+'|'-'] Rel ( ('==' | '!=') Rel )*
DATA_TYPE Diagram::Expr() {
    int t = peekToken();
    bool hasUnary = false;
    string unaryOp = "";

    // Пропускаем унарные операции для констант - они обрабатываются в Prim()
    // Оставляем только для случаев, когда это не константа
    if (t == PLUS || t == MINUS) {
        // Смотрим вперед, чтобы определить, константа ли это
        int savedTok = nextToken();
        string savedLex = curLex;
        int nextTok = peekToken();

        // Если следующий токен - константа, то унарную операцию обработает Prim()
        if (nextTok == DEC_CONST || nextTok == HEX_CONST) {
            pushBack(savedTok, savedLex);
        }
        else {
            // Если не константа, то обрабатываем как унарную операцию
            hasUnary = true;
            unaryOp = (savedTok == PLUS) ? "+" : "-";
        }
    }

    DATA_TYPE left = Rel();

    // Обработка унарной операции (только для не-констант)
    if (hasUnary) {
        if (!(left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT)) {
            semError("унарный '+'/'-' применим только к целым типам");
        }

        SemNode operand = popValue();
        SemNode result;

        if (unaryOp == "-") {
            SemNode minusOne;
            minusOne.DataType = left;
            minusOne.hasValue = true;

            switch (left) {
            case TYPE_SHORT_INT: minusOne.Value.v_int16 = -1; break;
            case TYPE_INT: minusOne.Value.v_int32 = -1; break;
            case TYPE_LONG_INT: minusOne.Value.v_int64 = -1; break;
            default: break;
            }

            result = Tree::executeArithmeticOp(operand, minusOne, "*", sc->getLineCol().first, sc->getLineCol().second);
        }
        else {
            result = operand;
        }

        pushValue(result);
        left = result.DataType;
    }

    t = peekToken();
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
            semError("операнды для '=='/'!=' должны быть одного типа");
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
            semError("операнды для '<, <=, >, >=' должны быть целыми (int/short/long)");
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
            semError("операнды для сдвигов должны быть целыми (int/short/long)");
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
            semError("операнды для '+'/'-' должны быть целыми (int/short/long)");
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
            semError("операнды для '*', '/', '%' должны быть целыми (int/short/long)");
        }

        SemNode result = Tree::executeArithmeticOp(leftVal, rightVal, op, sc->getLineCol().first, sc->getLineCol().second);
        pushValue(result);
        left = result.DataType;
        t = peekToken();
    }
    return left;
}

// Prim -> IDENT | Const | '(' Expr ')'
DATA_TYPE Diagram::Prim() {
    int t = nextToken();

    // Сначала проверяем унарный минус для отрицательных констант
    if (t == MINUS) {
        t = nextToken();
        if (t == DEC_CONST || t == HEX_CONST) {
            DATA_TYPE constType = TYPE_SHORT_INT;

            // Определяем, нужно ли автоматически повышать тип до long
            string valueStr = "-" + curLex;
            try {
                long long val = std::stoll(valueStr, nullptr, 0);

                // Если значение выходит за пределы short, автоматически делаем его int
                if (val > 32767 || val < -32768) {
                    constType = TYPE_INT;
                }

                // Если значение выходит за пределы int, автоматически делаем его long
                if (val > 2147483647LL || val < -2147483648LL) {
                    constType = TYPE_LONG_INT;
                }
            }
            catch (...) {
                // Оставляем как TYPE_INT в случае ошибки
            }

            SemNode constNode = evaluateConstant("-" + curLex, constType);
            pushValue(constNode);
            return constType;
        }
        else {
            // Если после минуса не константа, то это унарная операция над выражением
            // Помещаем токен обратно и обрабатываем как обычное выражение в скобках
            pushBack(t, curLex);
            pushBack(MINUS, "-");
            // Обрабатываем как выражение в скобках
            t = nextToken();
            if (t != LPAREN) {
                synError("ожидалась константа или выражение в скобках после '-'");
            }
            DATA_TYPE dt = Expr();
            t = nextToken();
            if (t != RPAREN) synError("ожидался ')' после выражения");
            return dt;
        }
    }

    if (t == DEC_CONST || t == HEX_CONST) {
        DATA_TYPE constType = TYPE_SHORT_INT;

        // Определяем, нужно ли автоматически повышать тип до long
        string valueStr = curLex;
        try {
            long long val = std::stoll(valueStr, nullptr, 0);

            // Если значение выходит за пределы short, автоматически делаем его int
            if (val > 32767 || val < -32768) {
                constType = TYPE_INT;
            }

            // Если значение выходит за пределы int, автоматически делаем его long
            if (val > 2147483647LL || val < -2147483648LL) {
                constType = TYPE_LONG_INT;
            }
        }
        catch (...) {
            // Оставляем как TYPE_INT в случае ошибки
        }

        SemNode constNode = evaluateConstant(curLex, constType);
        pushValue(constNode);
        return constType;
    }

    if (t == KW_TRUE || t == KW_FALSE) {
        DATA_TYPE boolType = TYPE_BOOL;
        SemNode boolNode;
        boolNode.DataType = TYPE_BOOL;
        boolNode.hasValue = true;
        boolNode.Value.v_bool = (t == KW_TRUE);
        pushValue(boolNode);
        return boolType;
    }

    if (t == LPAREN) {
        DATA_TYPE dt = Expr();
        t = nextToken();
        if (t != RPAREN) synError("ожидался ')' после выражения");
        return dt;
    }

    if (t == IDENT) {
        string name = curLex;
        int t2 = peekToken();

        if (t2 == LPAREN) {
            semError("вызов функции внутри выражения невозможен: функции возвращают void");
        }
        else {
            auto lc = sc->getLineCol();
            Tree* v = Tree::Cur->semGetVar(name, lc.first, lc.second);

            if (!v->n->hasValue) {
                interpError("использование неинициализированной переменной '" + name + "'");
            }

            SemNode value;
            value.DataType = v->n->DataType;
            value.hasValue = true;
            value.Value = v->n->Value;
            pushValue(value);

            return v->n->DataType;
        }
    }

    synError("ожидалось первичное выражение (IDENT, константа или скобки)");
    return TYPE_INT;
}

// Const -> DEC_CONST | HEX_CONST | true | false
void Diagram::Const() {
    int t = nextToken();
    if (!(t == DEC_CONST || t == HEX_CONST || t == KW_TRUE || t == KW_FALSE))
        synError("ожидалась константа");
}

// Name -> IDENT
void Diagram::Name() {
    int t = nextToken();
    if (t != IDENT) synError("ожидался идентификатор (имя)");
}