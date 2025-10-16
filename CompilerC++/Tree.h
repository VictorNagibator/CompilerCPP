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
    void SetLeft(SemNode* Data); // вставить как первый дочерний элемент текущего узла
    void SetRight(SemNode* Data); // вставить как правого соседа текущего узла 

    // поиск в дереве
    Tree* FindUp(Tree* From, const string& id); // поиск в текущей и внешних областях
    Tree* FindUpOneLevel(Tree* From, const string& id); // поиск только в текущем уровне

    // Семантические операции
    // занесение идентификатора a в текущую область
    Tree* SemInclude(const string& a, DATA_TYPE t, int line, int col);

    // установить число формальных параметров для функции
    void SemSetParam(Tree* Addr, int n);

    // установить список типов формальных параметров для функции
    void SemSetParamTypes(Tree* Addr, const std::vector<DATA_TYPE>& types);

    // проверить фактические параметры по числу и типам (вызывать при Call)
    void SemControlParamTypes(Tree* Addr, const std::vector<DATA_TYPE>& argTypes, int line, int col);

    // найти переменную (не функцию) с именем a в видимых областях
    Tree* SemGetVar(const string& a, int line, int col);

    // найти функцию с именем a
    Tree* SemGetFunct(const string& a, int line, int col);

    // проверка дубля на текущем уровне
    bool DupControl(Tree* Addr, const string& a);

    // Вход/выход в/из области (составной оператор)
    Tree* SemEnterBlock(int line, int col);
    void SemExitBlock();

    // Установка/получение текущей вершины
    static void SetCur(Tree* a) { Cur = a; }
    static Tree* GetCur() { return Cur; }

    void Print(); // Упрощенная версия

private:
    // Вспомогательная печать ошибки и остановка
    static void SemError(const char* msg, const string& id = "", int line = -1, int col = -1);

    // Печать дерева
    void Print(int depth);
	std::string makeLabel(const Tree* tree) const;
};