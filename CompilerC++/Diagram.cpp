#include "Diagram.h"
#include "Tree.h"
#include <iostream>

// Конструктор
Diagram::Diagram(Scanner* scanner) : sc(scanner), curTok(0), curLex(), currentDeclType(TYPE_INT) {
    pushTok.clear();
    pushLex.clear();
}

// Вспомогательные: вывод ошибок с форматом и выход
void Diagram::synError(const string& msg) {
    auto lc = sc->getLineCol();
    std::cerr << "Синтаксическая ошибка: " << msg;
    if (!curLex.empty()) std::cerr << " (около '" << curLex << "')";
    std::cerr << std::endl << "(строка " << lc.first << ":" << lc.second << ")" << std::endl;
    std::exit(1);
}

void Diagram::semError(const string& msg) {
    auto lc = sc->getLineCol();
    std::cerr << "Семантическая ошибка: " << msg;
    if (!curLex.empty()) std::cerr << " (около '" << curLex << "')";
    std::cerr << std::endl << "(строка " << lc.first << ":" << lc.second << ")" << std::endl;
    std::exit(1);
}

// Возврат следующего токена (с контролем лексической ошибки)
int Diagram::nextToken() {
    if (!pushTok.empty()) {
        int t = pushTok.back();
        pushTok.pop_back();
        string lx = pushLex.back();
        pushLex.pop_back();
        curTok = t;
        curLex = lx;
        if (curTok == T_ERR) {
            auto lc = sc->getLineCol();
            std::cerr << "Лексическая ошибка: неизвестная лексема '" << curLex << "'\n";
            std::cerr << "(строка " << lc.first << ":" << lc.second << ")\n";
            std::exit(1);
        }
        return curTok;
    }

    string lex;
    curTok = sc->getNextLex(lex);
    curLex = lex;

    if (curTok == T_ERR) {
        auto lc = sc->getLineCol();
        std::cerr << "Лексическая ошибка: неизвестная лексема '" << curLex << "'\n";
        std::cerr << "(строка " << lc.first << ":" << lc.second << ")\n";
        std::exit(1);
    }
    return curTok;
}

// Посмотреть следующий токен (не читая)
int Diagram::peekToken() {
    int t = nextToken();
    pushBack(t, curLex);
    return t;
}

// Вернуть токен в поток
void Diagram::pushBack(int tok, const string& lex) {
    pushTok.push_back(tok);
    pushLex.push_back(lex);
}

// Точка входа
void Diagram::ParseProgram() {
    // создаём корень семантического дерева (область верхнего уровня)
    SemNode* rootNode = new SemNode();
    rootNode->id = "<корень>";
    rootNode->DataType = TYPE_SCOPE;
    rootNode->line = 0;
    rootNode->col = 0;
    Tree* rootTree = new Tree(rootNode, nullptr);
    Tree::SetCur(rootTree);

    Program();

    // проверим, что в конце файла действительно конец
    int t = nextToken();
    if (t != T_END) synError("лишний текст в конце программы");
}

// Program -> TopDecl*
void Diagram::Program() {
    int t = peekToken();
    while (t == KW_VOID || t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL) {
        TopDecl();
        t = peekToken();
    }

    if (t != T_END) {
        synError("ожидалось объявление переменной или определение функции");
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

    // имя функции
    t = nextToken();
    if (t != IDENT && t != KW_MAIN) synError("ожидалось имя функции (IDENT)");
    string funcName = curLex;
    auto pos = sc->getLineCol();

    // Семантика: занести функцию в семантическое дерево
    Tree* funcNode = Tree::Cur->SemInclude(funcName, TYPE_FUNCT, pos.first, pos.second);

    // Запомним текущую область, чтобы потом восстановить
    Tree* savedCur = Tree::Cur;

    // Переключаем Cur на тело функции (правый узел)
    if (funcNode && funcNode->Right) {
        Tree::SetCur(funcNode->Right);
    }
    else {
        // Защита на случай ошибки
        Tree::Cur->SemEnterBlock(pos.first, pos.second);
    }

    // '('
    t = nextToken();
    if (t != LPAREN) synError("ожидался '(' после имени функции");

    // Разбор списка параметров
    std::vector<DATA_TYPE> paramTypes;
    int paramCount = 0;
    t = peekToken();
    if (t != RPAREN) {
        for (;;) {
            // Type
            t = nextToken();
            if (!(t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL))
                synError("ожидался тип параметра");

            DATA_TYPE ptype = TYPE_INT;
            if (t == KW_SHORT) ptype = TYPE_SHORT_INT;
            else if (t == KW_LONG) ptype = TYPE_LONG_INT;
            else if (t == KW_BOOL) ptype = TYPE_BOOL;

            // IDENT
            t = nextToken();
            if (t != IDENT) synError("ожидалось имя параметра");
            string paramName = curLex;
            auto ppos = sc->getLineCol();

            // Семантика: занести параметр в текущую область
            Tree::Cur->SemInclude(paramName, ptype, ppos.first, ppos.second);

            paramTypes.push_back(ptype);
            paramCount++;

            // если запятая — продолжаем, иначе выходим
            t = peekToken();
            if (t == COMMA) {
                nextToken(); // съесть ','
                continue;
            }
            else {
                break;
            }
        }
    }

    // ')'
    t = nextToken();
    if (t != RPAREN) synError("ожидался ')' после списка параметров");

    // Записать информацию о параметрах в узел функции
    funcNode->SemSetParam(funcNode, paramCount);
    funcNode->SemSetParamTypes(funcNode, paramTypes);

    // Тело функции
    Block();

    // Восстанавливаем предыдущую область
    Tree::SetCur(savedCur);
}

// VarDecl -> Type IdInitList ;
void Diagram::VarDecl() {
    int t = nextToken();
    if (!(t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL))
        synError("ожидался тип в объявлении переменных");

    // Определяем DATA_TYPE
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
        nextToken(); // ','
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

    // Семантика: занести идентификатор в текущую область
    Tree* varNode = Tree::Cur->SemInclude(varName, currentDeclType, pos.first, pos.second);

    t = peekToken();
    if (t == ASSIGN) {
        nextToken(); // считать '='

        // Проверка инициализации
        DATA_TYPE exprType = Expr();
        DATA_TYPE varType = varNode->n->DataType;

        // Проверка совместимости типов
        bool varIsInt = (varType == TYPE_INT || varType == TYPE_SHORT_INT || varType == TYPE_LONG_INT);
        bool exprIsInt = (exprType == TYPE_INT || exprType == TYPE_SHORT_INT || exprType == TYPE_LONG_INT);

        if (!((varIsInt && exprIsInt) || (varType == TYPE_BOOL && exprType == TYPE_BOOL))) {
            semError("несоответствие типов при инициализации переменной '" + varName + "'");
        }
    }
}

// Block -> '{' BlockItems '}'
void Diagram::Block() {
    int t = nextToken();
    if (t != LBRACE) synError("ожидался '{' для начала блока");

    // Семантика: вход в новую область
    auto lc = sc->getLineCol();
    Tree::Cur->SemEnterBlock(lc.first, lc.second);

    BlockItems();

    t = nextToken();
    if (t != RBRACE) synError("ожидался '}' для конца блока");

    // Выход из области
    Tree::Cur->SemExitBlock();
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
        // Нужно определить: присваивание или вызов функции
        int tokIdent = nextToken();
        string savedName = curLex; // СОХРАНИТЬ лексему ДО peekToken!
        int t2 = peekToken();

        // Возвращаем идентификатор обратно с ПРАВИЛЬНОЙ лексемой
        pushBack(tokIdent, savedName);

        if (t2 == ASSIGN) {
            Assign();
            // Проверяем ';'
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

    // Проверяем ';'
    int t = nextToken();
    if (t != SEMI) synError("ожидался ';' после вызова функции");
}

// Assign -> IDENT = Expr
void Diagram::Assign() {
    int t = nextToken();
    if (t != IDENT) synError("ожидался идентификатор в присваивании");
    string name = curLex;
    auto lc = sc->getLineCol();

    // Семантика: проверка левой части
    Tree* leftNode = Tree::Cur->SemGetVar(name, lc.first, lc.second);
    if (leftNode->n->DataType == TYPE_FUNCT) {
        semError("нельзя присваивать функции '" + name + "'");
    }

    // '='
    t = nextToken();
    if (t != ASSIGN) synError("ожидался знак '=' в присваивании");

    // Проверка правой части
    DATA_TYPE rtype = Expr();
    DATA_TYPE ltype = leftNode->n->DataType;

    // Проверка совместимости типов
    bool lIsInt = (ltype == TYPE_INT || ltype == TYPE_SHORT_INT || ltype == TYPE_LONG_INT);
    bool rIsInt = (rtype == TYPE_INT || rtype == TYPE_SHORT_INT || rtype == TYPE_LONG_INT);

    if (!((lIsInt && rIsInt) || (ltype == TYPE_BOOL && rtype == TYPE_BOOL))) {
        semError("несоответствие типов при присваивании для '" + name + "'");
    }
}

// SwitchStmt -> 'switch' '(' Expr ')' '{' CaseStmt* DefaultStmt? '}'
void Diagram::SwitchStmt() {
    int t = nextToken();
    if (t != KW_SWITCH) synError("ожидался 'switch'");

    t = nextToken();
    if (t != LPAREN) synError("ожидался '(' после 'switch'");

    // Проверка типа выражения в switch
    DATA_TYPE stype = Expr();
    if (!(stype == TYPE_INT || stype == TYPE_SHORT_INT || stype == TYPE_LONG_INT)) {
        semError("тип выражения в switch должен быть целым (int)");
    }

    t = nextToken();
    if (t != RPAREN) synError("ожидался ')' после выражения switch");

    t = nextToken();
    if (t != LBRACE) synError("ожидался '{' для тела switch");

    // Разбираем case-последовательность
    t = peekToken();
    while (t == KW_CASE) {
        CaseStmt();
        t = peekToken();
    }

    // Опциональный default
    if (peekToken() == KW_DEFAULT) {
        DefaultStmt();
    }

    t = nextToken();
    if (t != RBRACE) synError("ожидался '}' в конце switch");
}

// CaseStmt -> 'case' Const ':' Stmt*
void Diagram::CaseStmt() {
    int t = nextToken();
    if (t != KW_CASE) synError("ожидался 'case'");

    // Проверка константы
    t = nextToken();
    if (!(t == DEC_CONST || t == HEX_CONST)) {
        semError("case принимает только числовую константу");
    }

    t = nextToken();
    if (t != COLON) synError("ожидался ':' после case-значения");

    // Тело case: произвольные операторы
    t = peekToken();
    while (t != KW_CASE && t != KW_DEFAULT && t != RBRACE && t != T_END) {
        Stmt();
        t = peekToken();
    }
}

// DefaultStmt -> 'default' ':' Stmt*
void Diagram::DefaultStmt() {
    int t = nextToken();
    if (t != KW_DEFAULT) synError("ожидался 'default'");

    t = nextToken();
    if (t != COLON) synError("ожидался ':' после default");

    // Тело default
    t = peekToken();
    while (t != RBRACE && t != T_END) {
        Stmt();
        t = peekToken();
    }
}

// Call -> IDENT '(' ArgListOpt ')'
void Diagram::Call() {
    int t = nextToken();
    if (t != IDENT) synError("ожидалось имя функции при вызове");
    string fname = curLex;
    auto lc = sc->getLineCol();

    // Семантика: имя должно быть функцией
    Tree* fnode = Tree::Cur->SemGetFunct(fname, lc.first, lc.second);

    t = nextToken();
    if (t != LPAREN) synError("ожидался '(' после имени функции");

    // Собираем типы фактических параметров
    std::vector<DATA_TYPE> argTypes;
    t = peekToken();
    if (t != RPAREN) {
        DATA_TYPE atype = Expr();
        argTypes.push_back(atype);
        t = peekToken();
        while (t == COMMA) {
            nextToken(); // ','
            atype = Expr();
            argTypes.push_back(atype);
            t = peekToken();
        }
    }

    t = nextToken();
    if (t != RPAREN) synError("ожидался ')' после списка аргументов");

    // Проверяем число и типы аргументов
    fnode->SemControlParamTypes(fnode, argTypes, lc.first, lc.second);
}

// Expr -> ['+'|'-'] Rel ( ('==' | '!=') Rel )*
DATA_TYPE Diagram::Expr() {
    int t = peekToken();
    if (t == PLUS || t == MINUS) {
        nextToken(); // унарный +/-
    }

    DATA_TYPE left = Rel();
    t = peekToken();

    while (t == EQ || t == NEQ) {
        nextToken(); // EQ/NEQ
        DATA_TYPE right = Rel();

        // Проверка типов для ==/!=
        bool leftIsInt = (left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT);
        bool rightIsInt = (right == TYPE_INT || right == TYPE_SHORT_INT || right == TYPE_LONG_INT);

        if ((leftIsInt && rightIsInt) || (left == TYPE_BOOL && right == TYPE_BOOL)) {
            left = TYPE_BOOL; // результат сравнения — bool
        }
        else {
            semError("операнды для '=='/ '!=' должны быть одного типа (int или bool)");
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
        nextToken();
        DATA_TYPE right = Shift();

        // Для <,<=,>,>= оба операнда должны быть целыми
        bool lInt = (left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT);
        bool rInt = (right == TYPE_INT || right == TYPE_SHORT_INT || right == TYPE_LONG_INT);

        if (!(lInt && rInt)) {
            semError("операнды для '<, <=, >, >=' должны быть целыми (int)");
        }
        left = TYPE_BOOL; // результат — bool
        t = peekToken();
    }
    return left;
}

// Shift -> Add ( ('<<' | '>>') Add )*
DATA_TYPE Diagram::Shift() {
    DATA_TYPE left = Add();
    int t = peekToken();

    while (t == SHL || t == SHR) {
        nextToken();
        DATA_TYPE right = Add();

        bool lInt = (left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT);
        bool rInt = (right == TYPE_INT || right == TYPE_SHORT_INT || right == TYPE_LONG_INT);

        if (!(lInt && rInt)) {
            semError("операнды для сдвигов должны быть целыми (int)");
        }
        left = TYPE_INT;
        t = peekToken();
    }
    return left;
}

// Add -> Mul ( ('+' | '-') Mul )*
DATA_TYPE Diagram::Add() {
    DATA_TYPE left = Mul();
    int t = peekToken();

    while (t == PLUS || t == MINUS) {
        nextToken();
        DATA_TYPE right = Mul();

        bool lInt = (left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT);
        bool rInt = (right == TYPE_INT || right == TYPE_SHORT_INT || right == TYPE_LONG_INT);

        if (!(lInt && rInt)) {
            semError("операнды для '+'/'-' должны быть целыми (int)");
        }
        left = TYPE_INT;
        t = peekToken();
    }
    return left;
}

// Mul -> Prim ( ('*' | '/' | '%') Prim )*
DATA_TYPE Diagram::Mul() {
    DATA_TYPE left = Prim();
    int t = peekToken();

    while (t == MULT || t == DIV || t == MOD) {
        nextToken();
        DATA_TYPE right = Prim();

        bool lInt = (left == TYPE_INT || left == TYPE_SHORT_INT || left == TYPE_LONG_INT);
        bool rInt = (right == TYPE_INT || right == TYPE_SHORT_INT || right == TYPE_LONG_INT);

        if (!(lInt && rInt)) {
            semError("операнды для '*', '/', '%' должны быть целыми (int)");
        }
        left = TYPE_INT;
        t = peekToken();
    }
    return left;
}

// Prim -> IDENT | Const | '(' Expr ')'
DATA_TYPE Diagram::Prim() {
    int t = nextToken();

    if (t == DEC_CONST || t == HEX_CONST) {
        return TYPE_INT;
    }

    if (t == KW_TRUE || t == KW_FALSE) {
        return TYPE_BOOL;
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
            // Вызов функции внутри выражения недопустим
            semError("вызов функции внутри выражения невозможен: функции возвращают void");
            return TYPE_INT; // unreachable
        }
        else {
            // Обычный идентификатор
            auto lc = sc->getLineCol();
            Tree* v = Tree::Cur->SemGetVar(name, lc.first, lc.second);
            return v->n->DataType;
        }
    }

    synError("ожидалось первичное выражение (IDENT, константа или скобки)");
    return TYPE_INT; // для компилятора
}

// ArgListOpt -> Expr (',' Expr)* | epsilon
void Diagram::ArgListOpt() {
    // Используется внутри Call
    Expr();
    int t = peekToken();
    while (t == COMMA) {
        nextToken();
        Expr();
        t = peekToken();
    }
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