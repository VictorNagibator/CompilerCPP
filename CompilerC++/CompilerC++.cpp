#include "Diagram.h"

#include <iostream>
#include <Windows.h>

int main(int argc, char** argv) {
    // Корректно отображаем русский язык в консоли
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    string fname = "input.txt";
    if (argc > 1) fname = argv[1];

    Scanner sc;
    if (!sc.loadFile(fname)) {
        cerr << "Не удалось открыть " << fname << endl;
        return 2;
    }

    Diagram dg(&sc);
    dg.ParseProgram();

    return 0;
}