// main.cpp
#include <iostream>
#include <string>
#include <Windows.h>

#include "Diagram.h"
#include "Tree.h"

using std::string;
using std::cout;
using std::cerr;
using std::endl;

int main(int argc, char** argv) {
    // Корректно отображаем русский язык в консоли
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    string fname = "input.txt";
    if (argc > 1) fname = argv[1];

    Scanner sc;
    if (!sc.loadFile(fname)) {
        cerr << "Ошибка: не удалось открыть файл: " << fname << endl;
        return 2;
    }

    // Разбор
    Diagram dg(&sc);
    dg.ParseProgram();

    // Если мы дошли сюда, то лексика, синтаксис и семантика прошли
    cout << "Синтаксических и семантических ошибок не обнаружено." << endl;
    cout << "Вывод семантического дерева:" << endl;
    if (Tree::Root) {
        Tree::Root->Print();
    }
    else {
        cout << "<дерево семантики пусто>" << endl;
    }

    return 0;
}
