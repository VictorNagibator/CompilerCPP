#include "Scanner.h"
#include "Defines.h"

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

    while (true) {
        string lex;
        int code = sc.getNextLex(lex);
        if (code == T_END) {
            cout << "T_END=" << code << "\n";
            break;
        }
        cout << lex << "  (код=" << code << ")\n";
        if (code == T_ERR) {
            cerr << "Лексическая ошибка: \"" << lex << "\"\n";
            break;
        }
    }
    return 0;
}