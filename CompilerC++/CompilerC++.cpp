#include <iostream>
#include <string>
#include <Windows.h>
#include "Diagram.h"

using namespace std;

int main(int argc, char** argv) {
    // Корректно отображаем русский язык в консоли
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    string fname = "input.txt";
    if (argc > 1) fname = argv[1];

    Scanner sc;
    if (!sc.loadFile(fname)) {
        cerr << "Ошибка: не удалось открыть файл: " << fname << endl;
        return -1;
    }

    // Разбор
    Diagram dg(&sc);
    dg.ParseProgram(true);

    return 0;
}
