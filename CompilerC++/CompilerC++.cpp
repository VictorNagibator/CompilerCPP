#include "Scanner.h"

#include <iostream>

int main(int argc, char** argv) {
    string fname = "input.txt";
    if (argc > 1) fname = argv[1];

    Scanner sc;
    if (!sc.loadFile(fname)) {
        cerr << "Не удалось открыть " << fname << endl;
        return 2;
    }

    while (true) {
        string lex;
        int tok = sc.getNextToken(lex);
        if (tok == T_END) {
            cout << "T_END=" << tok << "\n";
            break;
        }
        cout << lex << "  (token=" << tok << ")\n";
        if (tok == T_ERR) {
            cerr << "Лексическая ошибка: \"" << lex << "\"\n";
            break;
        }
    }
    return 0;
}