#include "Diagram.h"

#include <iostream>

using std::string;

// Конструктор
Diagram::Diagram(Scanner* scanner) : sc(scanner), curTok(0), curLex() {
    pushTok.clear();
    pushLex.clear();
}

// Возврат следующего токена
// результат сохраняется в curTok/curLex
int Diagram::nextToken() {
    if (!pushTok.empty()) {
        int t = pushTok.back(); pushTok.pop_back();
        string lx = pushLex.back(); pushLex.pop_back();
        curTok = t; curLex = lx;
        return curTok;
    }
    string lex;
    curTok = sc->getNextLex(lex);
    curLex = lex;
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

// Сообщение об ошибке и завершение
void Diagram::error(string msg) {
    std::cerr << "Синтаксическая ошибка: " << msg;
    if (!curLex.empty()) std::cerr << " (около '" << curLex << "')";
    std::cerr << std::endl;
    std::exit(1);
}

// Точка входа
void Diagram::ParseProgram() {
    Program();
}

// Реализация диаграмм

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
    if (t == KW_VOID) Function();
    else VarDecl();
}

// Function -> 'void' IDENT '(' ParamListOpt ')' Block
void Diagram::Function() {
    int t = nextToken();
    if (t != KW_VOID) error("ожидался 'void' в определении функции");
    t = nextToken();
    if (t != IDENT && t != KW_MAIN) error("ожидалось имя функции (IDENT)");
    t = nextToken();
    if (t != LPAREN) error("ожидался '(' после имени функции");

    // Параметры опциональны
    t = peekToken();
    if (t != RPAREN) {
        // Первый параметр: Type IDENT
        t = nextToken();
        if (!(t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL))
            error("ожидался тип параметра");
        t = nextToken();
        if (t != IDENT) error("ожидалось имя параметра");
        // Дополнительные параметры через запятую
        t = peekToken();
        while (t == COMMA) {
            nextToken(); // ','
            t = nextToken(); // тип
            if (!(t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL))
                error("ожидался тип параметра");
            t = nextToken(); // имя параметра
            if (t != IDENT) error("ожидалось имя параметра");
            t = peekToken();
        }
    }

    t = nextToken();
    if (t != RPAREN) error("ожидался ')' после списка параметров");

    // Тело функции — составной оператор
    Block();
}

// VarDecl -> Type IdInitList ;
void Diagram::VarDecl() {
    int t = nextToken();
    if (!(t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL))
        error("ожидался тип в объявлении переменных");
    IdInitList();
    t = nextToken();
    if (t != SEMI) error("ожидался ';' в конце объявления переменных");
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
    if (t != IDENT) error("ожидался идентификатор в списке объявлений");
    t = peekToken();
    if (t == ASSIGN) {
        nextToken(); // считать '='
        Expr();
    }
}

// Block -> '{' BlockItems '}'
void Diagram::Block() {
    int t = nextToken();
    if (t != LBRACE) error("ожидался '{' для начала блока");
    BlockItems();
    t = nextToken();
    if (t != RBRACE) error("ожидался '}' для конца блока");
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

// Stmt -> ';' | Block | Assign | SwitchStmt
void Diagram::Stmt() {
    int t = peekToken();
    if (t == SEMI) { nextToken(); return; }
    if (t == LBRACE) { Block(); return; }
    if (t == KW_SWITCH) { SwitchStmt(); return; }

    // Вариант присваивания: начинается с IDENT
    if (t == IDENT) {
        int tokIdent = nextToken(); // читаем IDENT (в curLex хранится имя)
        int t2 = peekToken(); // смотрим следующую лексему

        // Вернём IDENT обратно — дальнейшие вызовы (Assign/Call) сами его прочитают
        pushBack(tokIdent, curLex);

        if (t2 == ASSIGN) {
            // Присваивание: IDENT = Expr ;
            Assign();
            t = nextToken();
            if (t != SEMI) error("ожидался ';' после оператора присваивания");
            return;
        }
        else if (t2 == LPAREN) {
            // Вызов функции как оператор: IDENT '(' ArgListOpt ')' ;
            // Разбираем вызов и затем ожидаем ';'
            Call();
            t = nextToken();
            if (t != SEMI) error("ожидался ';' после вызова функции");
            return;
        }
        else {
            error("ожидалось '=' (присваивание) или '(' (вызов функции) после идентификатора");
        }
    }

    error("неизвестная форма оператора");
}

// Assign -> IDENT = Expr ; (вызвать Assign() можно — ';' проверяется выше)
void Diagram::Assign() {
    int t = nextToken();
    if (t != IDENT) error("ожидался идентификатор в присваивании");
    t = nextToken();
    if (t != ASSIGN) error("ожидался знак '=' в присваивании");
    Expr();
    // ';' снимается вызывающим кодом (здесь не проверяем)
}

// SwitchStmt -> 'switch' '(' Expr ')' '{' (case ...)* [ default : ... ] '}'
void Diagram::SwitchStmt() {
    int t = nextToken();
    if (t != KW_SWITCH) error("ожидался 'switch'");
    t = nextToken();
    if (t != LPAREN) error("ожидался '(' после 'switch'");
    Expr();
    t = nextToken();
    if (t != RPAREN) error("ожидался ')' после выражения switch");
    t = nextToken();
    if (t != LBRACE) error("ожидался '{' для тела switch");

    // разбираем case-последовательность
    t = peekToken();
    while (t == KW_CASE) {
        nextToken(); // consume 'case'
        t = nextToken();
        if (!(t == DEC_CONST || t == HEX_CONST || t == KW_TRUE || t == KW_FALSE)) {
            error("ожидалась константа после 'case'");
        }
        t = nextToken();
        if (t != COLON) error("ожидался ':' после case-значения");

        // тело case: CaseItem* (Stmt | break ;)
        t = peekToken();
        while (t != KW_CASE && t != KW_DEFAULT && t != RBRACE && t != T_END) {
            if (t == KW_BREAK) {
                nextToken(); // 'break'
                t = nextToken();
                if (t != SEMI) error("ожидался ';' после break");
            }
            else {
                Stmt();
            }
            t = peekToken();
        }
        t = peekToken();
    }

    // опциональный default (только один и после всех case)
    if (peekToken() == KW_DEFAULT) {
        nextToken(); // default
        t = nextToken();
        if (t != COLON) error("ожидался ':' после default");
        // CaseItem* как и выше
        t = peekToken();
        while (t != RBRACE && t != T_END) {
            if (t == KW_BREAK) {
                nextToken();
                t = nextToken();
                if (t != SEMI) error("ожидался ';' после break");
            }
            else if (t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL) {
                VarDecl();
            }
            else {
                Stmt();
            }
            t = peekToken();
        }
    }

    t = nextToken();
    if (t != RBRACE) error("ожидался '}' в конце switch");
}

// Name -> IDENT  (в этом варианте имя — просто IDENT)
void Diagram::Name() {
    int t = nextToken();
    if (t != IDENT) error("ожидался идентификатор (имя)");
}

// Expr -> ['+'|'-'] Rel ( ('==' | '!=') Rel )*
void Diagram::Expr() {
    int t = peekToken();
    if (t == PLUS || t == MINUS) nextToken(); // унарный +/-
    Rel();
    t = peekToken();
    while (t == EQ || t == NEQ) {
        nextToken(); // съели EQ/NEQ :)
        Rel();
        t = peekToken();
    }
}

// Rel -> Shift ( ('<' | '<=' | '>' | '>=') Shift )*
void Diagram::Rel() {
    Shift();
    int t = peekToken();
    while (t == LT || t == LE || t == GT || t == GE) {
        nextToken();
        Shift();
        t = peekToken();
    }
}

// Shift -> Add ( ('<<' | '>>') Add )*
void Diagram::Shift() {
    Add();
    int t = peekToken();
    while (t == SHL || t == SHR) {
        nextToken();
        Add();
        t = peekToken();
    }
}

// Add -> Mul ( ('+' | '-') Mul )*
void Diagram::Add() {
    Mul();
    int t = peekToken();
    while (t == PLUS || t == MINUS) {
        nextToken();
        Mul();
        t = peekToken();
    }
}

// Mul -> Prim ( ('*' | '/' | '%') Prim )*
void Diagram::Mul() {
    Prim();
    int t = peekToken();
    while (t == MULT || t == DIV || t == MOD) {
        nextToken();
        Prim();
        t = peekToken();
    }
}

// Prim -> IDENT | Const | IDENT '(' ArgListOpt ')' | '(' Expr ')'
void Diagram::Prim() {
    int t = nextToken();
    if (t == DEC_CONST || t == HEX_CONST || t == KW_TRUE || t == KW_FALSE || t == BOOL_CONST) {
        return; // константа
    }
    if (t == LPAREN) {
        Expr();
        t = nextToken();
        if (t != RPAREN) error("ожидался ')' после выражения");
        return;
    }
    if (t == IDENT) {
        int t2 = peekToken();
        if (t2 == LPAREN) {
            // IDENT и разобрать как вызов функции через Call()
            pushBack(t, curLex);
            Call();
            return;
        }
        else {
            // простое имя
            return;
        }
    }
    error("ожидалось первичное выражение (IDENT, константа или скобки)");
}

// Call -> IDENT '(' ArgListOpt ')'
void Diagram::Call() {
    int t = nextToken();
    if (t != IDENT) error("ожидалось имя функции при вызове");
    t = nextToken();
    if (t != LPAREN) error("ожидался '(' после имени функции");
    t = peekToken();
    if (t != RPAREN) {
        ArgListOpt();
    }
    t = nextToken();
    if (t != RPAREN) error("ожидался ')' после списка аргументов");
}

// ArgListOpt -> Expr (',' Expr)* | epsilon
void Diagram::ArgListOpt() {
    Expr();
    int t = peekToken();
    while (t == COMMA) {
        nextToken(); // ','
        Expr();
        t = peekToken();
    }
}

// Const -> DEC_CONST | HEX_CONST | true | false
void Diagram::Const() {
    int t = nextToken();
    if (!(t == DEC_CONST || t == HEX_CONST || t == KW_TRUE || t == KW_FALSE || t == BOOL_CONST))
        error("ожидалась константа");
}