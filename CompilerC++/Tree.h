#pragma once
#include "SemNode.h"
#include <fstream>

// Класс семантического дерева (области видимости)
class Tree {
public:
    SemNode* n; // данные узла
    Tree* Up; // родитель (внешняя область)
    Tree* Left; // первый вложенный элемент (левая ссылка)
    Tree* Right; // следующий элемент на том же уровне (правый сосед)

    // Текущая позиция (корень/текущий блок)
    static Tree* Root;
    static Tree* Cur;

    // Конструктор и деструктор
    Tree(SemNode* node = nullptr, Tree* up = nullptr);
    ~Tree();

    // вставка левого/правого дочернего (создают новый узел)
    void setLeft(SemNode* Data); // вставить как первый дочерний элемент текущего узла
    void setRight(SemNode* Data); // вставить как правого соседа текущего узла 

    // поиск в дереве
    Tree* findUp(Tree* From, const string& id); // поиск в текущей и внешних областях
    Tree* findUpOneLevel(Tree* From, const string& id); // поиск только в текущем уровне

    // Семантические операции
    // занесение идентификатора a в текущую область
    Tree* semInclude(const string& a, DATA_TYPE t, int line, int col);

    // установить число формальных параметров для функции
    void semSetParam(Tree* Addr, int n);

    // установить список типов формальных параметров для функции
    void semSetParamTypes(Tree* Addr, const std::vector<DATA_TYPE>& types);

    // проверить фактические параметры по числу и типам (вызывать при Call)
    void semControlParamTypes(Tree* Addr, const std::vector<DATA_TYPE>& argTypes, int line, int col);

    // найти переменную (не функцию) с именем a в видимых областях
    Tree* semGetVar(const string& a, int line, int col);

    // найти функцию с именем a
    Tree* semGetFunct(const string& a, int line, int col);

    // проверка дубля на текущем уровне
    bool dupControl(Tree* Addr, const string& a);

    // Вход/выход в/из области (составной оператор)
    Tree* semEnterBlock(int line, int col);
    void semExitBlock();

    // Установка/получение текущей вершины
    static void setCur(Tree* a) { Cur = a; }
    static Tree* getCur() { return Cur; }

    void print(); // Упрощенная версия печати дерева

    // Печать ошибки и остановка
    static void semError(const string& msg, const string& id = "", int line = -1, int col = -1);

private:
    // Печать дерева
    void print(int depth);
	std::string makeLabel(const Tree* tree) const;
};