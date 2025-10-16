#pragma once
#include "SemNode.h"

class Tree {
public:
    SemNode* n;    // данные узла
    Tree* Up;      // родитель (внешняя область)
    Tree* Left;    // первый вложенный элемент (левая ссылка)
    Tree* Right;   // следующий элемент на том же уровне (правый сосед)

    // Текущая позиция (корень/текущий блок) -- статическая
    static Tree* Root;
    static Tree* Cur;

    // Конструкторы / деструктор
    Tree(SemNode* node = nullptr, Tree* up = nullptr);
    ~Tree();

    // Управление деревом: вставка левого/правого дочернего (создают новый узел)
    void SetLeft(SemNode* Data);   // вставить как первый дочерний элемент текущего узла
    void SetRight(SemNode* Data);  // вставить как правого соседа текущего узла 

    // Поиск: блочная видимость
    Tree* FindUp(Tree* From, const string& id);        // поиск в текущей и внешних областях
    Tree* FindUpOneLevel(Tree* From, const string& id);// поиск только в текущем уровне (среди детей From)

    // Семантические операции
    // занесение идентификатора a в текущую область; возвращает указатель на созданный узел (Tree *)
    // если t == TYPE_FUNCT - создаётся узел функции и у него создаётся правый пустой узел для тела,
    // возвращается указатель на узел функции (не на пустой правый узел)
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

    // проверка дубля на текущем уровне (возвращает 1 — дубль, 0 — нет)
    int DupControl(Tree* Addr, const string& a);

    // Вход/выход в/из области (составной оператор)
    // SemEnterBlock создаёт анонимный узел области под Cur и переключает Cur на него
    Tree* SemEnterBlock(int line, int col);
    void SemExitBlock();

    // Установка/получение текущей вершины
    static void SetCur(Tree* a) { Cur = a; }
    static Tree* GetCur() { return Cur; }

    // Печать дерева (отладка)
    void Print(int level, const std::string& prefix, bool isLast);
    void Print(); // Упрощенная версия

private:
    // Вспомогательная печать ошибки и остановка
    static void SemError(const char* msg, const string& id = "", int line = -1, int col = -1);
};