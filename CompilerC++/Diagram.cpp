#include "Diagram.h"

#include <iostream>

using std::string;

// �����������
Diagram::Diagram(Scanner* scanner) : sc(scanner), curTok(0), curLex() {
    pushTok.clear();
    pushLex.clear();
}

// ������� ���������� ������
// ��������� ����������� � curTok/curLex
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

// ���������� ��������� ����� (�� �����)
int Diagram::peekToken() {
    int t = nextToken();
    pushBack(t, curLex);
    return t;
}

// ������� ����� � �����
void Diagram::pushBack(int tok, const string& lex) {
    pushTok.push_back(tok);
    pushLex.push_back(lex);
}

// ��������� �� ������ � ����������
void Diagram::error(string msg) {
    std::cerr << "�������������� ������: " << msg;
    if (!curLex.empty()) std::cerr << " (����� '" << curLex << "')";
    std::cerr << std::endl;
    std::exit(1);
}

// ����� �����
void Diagram::ParseProgram() {
    Program();
}

// ���������� ��������

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
    if (t != KW_VOID) error("�������� 'void' � ����������� �������");
    t = nextToken();
    if (t != IDENT && t != KW_MAIN) error("��������� ��� ������� (IDENT)");
    t = nextToken();
    if (t != LPAREN) error("�������� '(' ����� ����� �������");

    // ��������� �����������
    t = peekToken();
    if (t != RPAREN) {
        // ������ ��������: Type IDENT
        t = nextToken();
        if (!(t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL))
            error("�������� ��� ���������");
        t = nextToken();
        if (t != IDENT) error("��������� ��� ���������");
        // �������������� ��������� ����� �������
        t = peekToken();
        while (t == COMMA) {
            nextToken(); // ','
            t = nextToken(); // ���
            if (!(t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL))
                error("�������� ��� ���������");
            t = nextToken(); // ��� ���������
            if (t != IDENT) error("��������� ��� ���������");
            t = peekToken();
        }
    }

    t = nextToken();
    if (t != RPAREN) error("�������� ')' ����� ������ ����������");

    // ���� ������� � ��������� ��������
    Block();
}

// VarDecl -> Type IdInitList ;
void Diagram::VarDecl() {
    int t = nextToken();
    if (!(t == KW_INT || t == KW_SHORT || t == KW_LONG || t == KW_BOOL))
        error("�������� ��� � ���������� ����������");
    IdInitList();
    t = nextToken();
    if (t != SEMI) error("�������� ';' � ����� ���������� ����������");
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
    if (t != IDENT) error("�������� ������������� � ������ ����������");
    t = peekToken();
    if (t == ASSIGN) {
        nextToken(); // ������� '='
        Expr();
    }
}

// Block -> '{' BlockItems '}'
void Diagram::Block() {
    int t = nextToken();
    if (t != LBRACE) error("�������� '{' ��� ������ �����");
    BlockItems();
    t = nextToken();
    if (t != RBRACE) error("�������� '}' ��� ����� �����");
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

    // ������� ������������: ���������� � IDENT
    if (t == IDENT) {
        int tokIdent = nextToken(); // ������ IDENT (� curLex �������� ���)
        int t2 = peekToken(); // ������� ��������� �������

        // ����� IDENT ������� � ���������� ������ (Assign/Call) ���� ��� ���������
        pushBack(tokIdent, curLex);

        if (t2 == ASSIGN) {
            // ������������: IDENT = Expr ;
            Assign();
            t = nextToken();
            if (t != SEMI) error("�������� ';' ����� ��������� ������������");
            return;
        }
        else if (t2 == LPAREN) {
            // ����� ������� ��� ��������: IDENT '(' ArgListOpt ')' ;
            // ��������� ����� � ����� ������� ';'
            Call();
            t = nextToken();
            if (t != SEMI) error("�������� ';' ����� ������ �������");
            return;
        }
        else {
            error("��������� '=' (������������) ��� '(' (����� �������) ����� ��������������");
        }
    }

    error("����������� ����� ���������");
}

// Assign -> IDENT = Expr ; (������� Assign() ����� � ';' ����������� ����)
void Diagram::Assign() {
    int t = nextToken();
    if (t != IDENT) error("�������� ������������� � ������������");
    t = nextToken();
    if (t != ASSIGN) error("�������� ���� '=' � ������������");
    Expr();
    // ';' ��������� ���������� ����� (����� �� ���������)
}

// SwitchStmt -> 'switch' '(' Expr ')' '{' (case ...)* [ default : ... ] '}'
void Diagram::SwitchStmt() {
    int t = nextToken();
    if (t != KW_SWITCH) error("�������� 'switch'");
    t = nextToken();
    if (t != LPAREN) error("�������� '(' ����� 'switch'");
    Expr();
    t = nextToken();
    if (t != RPAREN) error("�������� ')' ����� ��������� switch");
    t = nextToken();
    if (t != LBRACE) error("�������� '{' ��� ���� switch");

    // ��������� case-������������������
    t = peekToken();
    while (t == KW_CASE) {
        nextToken(); // consume 'case'
        t = nextToken();
        if (!(t == DEC_CONST || t == HEX_CONST || t == KW_TRUE || t == KW_FALSE)) {
            error("��������� ��������� ����� 'case'");
        }
        t = nextToken();
        if (t != COLON) error("�������� ':' ����� case-��������");

        // ���� case: CaseItem* (Stmt | break ;)
        t = peekToken();
        while (t != KW_CASE && t != KW_DEFAULT && t != RBRACE && t != T_END) {
            if (t == KW_BREAK) {
                nextToken(); // 'break'
                t = nextToken();
                if (t != SEMI) error("�������� ';' ����� break");
            }
            else {
                Stmt();
            }
            t = peekToken();
        }
        t = peekToken();
    }

    // ������������ default (������ ���� � ����� ���� case)
    if (peekToken() == KW_DEFAULT) {
        nextToken(); // default
        t = nextToken();
        if (t != COLON) error("�������� ':' ����� default");
        // CaseItem* ��� � ����
        t = peekToken();
        while (t != RBRACE && t != T_END) {
            if (t == KW_BREAK) {
                nextToken();
                t = nextToken();
                if (t != SEMI) error("�������� ';' ����� break");
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
    if (t != RBRACE) error("�������� '}' � ����� switch");
}

// Name -> IDENT  (� ���� �������� ��� � ������ IDENT)
void Diagram::Name() {
    int t = nextToken();
    if (t != IDENT) error("�������� ������������� (���)");
}

// Expr -> ['+'|'-'] Rel ( ('==' | '!=') Rel )*
void Diagram::Expr() {
    int t = peekToken();
    if (t == PLUS || t == MINUS) nextToken(); // ������� +/-
    Rel();
    t = peekToken();
    while (t == EQ || t == NEQ) {
        nextToken(); // ����� EQ/NEQ :)
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
        return; // ���������
    }
    if (t == LPAREN) {
        Expr();
        t = nextToken();
        if (t != RPAREN) error("�������� ')' ����� ���������");
        return;
    }
    if (t == IDENT) {
        int t2 = peekToken();
        if (t2 == LPAREN) {
            // IDENT � ��������� ��� ����� ������� ����� Call()
            pushBack(t, curLex);
            Call();
            return;
        }
        else {
            // ������� ���
            return;
        }
    }
    error("��������� ��������� ��������� (IDENT, ��������� ��� ������)");
}

// Call -> IDENT '(' ArgListOpt ')'
void Diagram::Call() {
    int t = nextToken();
    if (t != IDENT) error("��������� ��� ������� ��� ������");
    t = nextToken();
    if (t != LPAREN) error("�������� '(' ����� ����� �������");
    t = peekToken();
    if (t != RPAREN) {
        ArgListOpt();
    }
    t = nextToken();
    if (t != RPAREN) error("�������� ')' ����� ������ ����������");
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
        error("��������� ���������");
}