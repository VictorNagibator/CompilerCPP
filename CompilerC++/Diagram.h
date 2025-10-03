#pragma once

#include "Scanner.h"
#include "Defines.h"
#include <string>
#include <vector>

using std::string;

class Diagram {
private:
    Scanner* sc;

    // буфер для токенов
    std::vector<int> pushTok;
    std::vector<string> pushLex;

    // последняя прочитанная лексема (для сообщений об ошибке)
    int curTok;
    string curLex;

    int nextToken(); // читать следующую лексему
    int peekToken(); // посмотреть токен, не читая
    void pushBack(int tok, const string& lex); // вернуть токен в поток
    void error(string msg); // вывести сообщение об ошибке и завершить

    // Синтаксические процедуры (имена — как в грамматике)
    void Program(); // верхнеуровневая программа (TopDecl*)
    void TopDecl(); // одно верхнеуровневое объявление (Function | VarDecl)
    void Function(); // разбор функции: void IDENT ( ParamListOpt ) Block
    void VarDecl(); // объявление переменных: Type IdInitList ;
    void IdInitList(); // список имен синициализацией: IdInit (',' IdInit)*
    void IdInit(); // один элемент списка: IDENT [= Expr]
    void Block(); // составной оператор: '{' BlockItems '}'
    void BlockItems(); // содержимое блока: (VarDecl | Stmt)*
    void Stmt(); // оператор: ';' | Block | Assign | Switch
    void Assign(); // присваивание: IDENT = Expr ;
    void SwitchStmt(); // switch (Expr) { case ... default ... }
    void Name(); // имя (IDENT)
    void Expr(); // выражение (уровень равенств; поддержка унарного +/−)
    void Rel(); // уровень отношений (<, <=, >, >=)
    void Shift(); // сдвиги (<<, >>)
    void Add(); // аддитивные (+, -)
    void Mul(); // мультипликативные (*, /, %)
    void Prim(); // первичное выражение: IDENT | CONST | IDENT(...) | (Expr)
    void Call(); // вызов функции: IDENT '(' ArgListOpt ')'
    void ArgListOpt(); // список аргументов
    void Const(); // терминальные константы

public:
    Diagram(Scanner* scanner);

    // Точка входа: разбор всей программы
    void ParseProgram(); // вызывает Program()
};