#include "scanner.h"
#include "defines.h"
#include <fstream>
#include <sstream>
#include <iostream>

#define MAX_CONST_LEN 20 // максимальная длина численной константы

// В конструкторе текущая позиция = 0, текст пуст
Scanner::Scanner() : text(), currentPos(0) {}

bool Scanner::loadFile(const string& fileName) {
    ifstream in(fileName);
    if (!in) return false;

    ostringstream ss;
    ss << in.rdbuf();
    text = ss.str();

    // Добавляем нулевой символ в конце текста (на всякий случай)
    text.push_back('\0');
    currentPos = 0;
    return true;
}

// Работа с текущим символом
char Scanner::peek(size_t offset) const {
    size_t pos = currentPos + offset;
    if (pos < text.size()) return text[pos];
    return '\0';
}
char Scanner::getChar() {
    if (currentPos < text.size()) return text[currentPos++];
    return '\0';
}
void Scanner::ungetChar() {
    if (currentPos > 0) --currentPos;
}

// Всевозможные классификации символов
bool Scanner::isLetter(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
bool Scanner::isDigit(char c) {
    return (c >= '0' && c <= '9');
}
bool Scanner::isHexDigit(char c) {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
bool Scanner::isIdentStart(char c) {
    return isLetter(c) || c == '_';
}
bool Scanner::isIdentPart(char c) {
    return isLetter(c) || isDigit(c) || c == '_';
}

// Проверка ключевых слов: возвращает соответствующий KW_* или IDENT
int Scanner::checkKeyword(const string& s) {
    if (s == "int") return KW_INT;
    if (s == "short") return KW_SHORT;
    if (s == "long") return KW_LONG;
    if (s == "bool") return KW_BOOL;
    if (s == "void") return KW_VOID;
    if (s == "switch") return KW_SWITCH;
    if (s == "case") return KW_CASE;
    if (s == "default") return KW_DEFAULT;
    if (s == "break") return KW_BREAK;
    if (s == "true") return KW_TRUE; // булевы константы распознаются как ключевые слова
    if (s == "false") return KW_FALSE;
    if (s == "main") return KW_MAIN;
    return IDENT;
}

// Пропустить пробелы и комментарии (// и /* ... */)
void Scanner::skipIgnored() {
    for (;;) {
        char c = peek();
        // пробелы / табуляция / перевод строки / возврат каретки
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            getChar();
            continue;
        }
        if (c == '/') {
            char n = peek(1);
            if (n == '/') {
                // однострочный комментарий: убираем '//' и читаем до следующей строки
                getChar(); getChar();
                while (peek() != '\n' && peek() != '\0') getChar();
                continue;
            }
            else if (n == '*') {
                // блочный комментарий /* ... */
                getChar(); getChar(); // убираем '/*'
                bool closed = false;
                while (peek() != '\0') {
                    char ch = getChar();
                    if (ch == '*' && peek() == '/') {
                        getChar(); // убираем '/'
                        closed = true;
                        break;
                    }
                }
                if (!closed) {
                    // незакрытый комментарий
                    return;
                }
                continue;
            }
        }
        break; // нет игнорируемых символов
    }
}

// Основной метод лексера: получить следующую лексему
int Scanner::getNextLex(string& outLex) {
    outLex.clear();

    // Пропускаем игнорируемые символы
    skipIgnored();

    // Конец текста
    char c = peek();
    if (c == '\0') { return T_END; }

    // Идентификатор или ключевое слово
    if (isIdentStart(c)) {
        string lex;
        lex.push_back(getChar());
        while (isIdentPart(peek())) lex.push_back(getChar());
        int code = checkKeyword(lex);
        outLex = lex;

        if (code == IDENT && lex.length() > MAX_CONST_LEN) return T_ERR;

        return code;
    }


    // Десятичные и шестнадацатеричные числа
    if (isDigit(c)) {
        char first = getChar();
        if (first == '0') {
            char nx = peek();
            if (nx == 'x' || nx == 'X') {
                // требуется >=1 16-ричная цифра
                getChar(); // убираем x/X
                string lex = "0";
                lex.push_back(nx);
                if (!isHexDigit(peek())) {
                    // ошибка: 0x без цифр
                    outLex = lex;
                    return T_ERR;
                }
                while (isHexDigit(peek())) lex.push_back(getChar());
                outLex = lex;

                if (lex.length() > MAX_CONST_LEN) return T_ERR;

                return HEX_CONST;
            }
            else if (isDigit(nx)) {
                // цифра после 0 - читаем как DEC
                string lex;
                lex.push_back(first);
                while (isDigit(peek())) lex.push_back(getChar());
                outLex = lex;

                if (lex.length() > MAX_CONST_LEN) return T_ERR;

                return DEC_CONST;
            }
            else {
                // просто '0'
                outLex = "0";
                return DEC_CONST;
            }
        }
        else {
            // первая цифра 1..9
            string lex;
            lex.push_back(first);
            while (isDigit(peek())) lex.push_back(getChar());
            outLex = lex;

            if (lex.length() > MAX_CONST_LEN) return T_ERR;

            return DEC_CONST;
        }
    }

    // Операторы и разделители
    char n1 = peek(1);
    switch (c) {
    case '+': getChar(); outLex = "+"; return PLUS;
    case '-': getChar(); outLex = "-"; return MINUS;
    case '*': getChar(); outLex = "*"; return MULT;
    case '%': getChar(); outLex = "%"; return MOD;
    case ';': getChar(); outLex = ";"; return SEMI;
    case ',': getChar(); outLex = ","; return COMMA;
    case '(': getChar(); outLex = "("; return LPAREN;
    case ')': getChar(); outLex = ")"; return RPAREN;
    case '{': getChar(); outLex = "{"; return LBRACE;
    case '}': getChar(); outLex = "}"; return RBRACE;
    case ':': getChar(); outLex = ":"; return COLON;
    case '=':
        getChar();
        if (peek() == '=') { getChar(); outLex = "=="; return EQ; }
        outLex = "="; return ASSIGN;
    case '!':
        getChar();
        if (peek() == '=') { getChar(); outLex = "!="; return NEQ; }
        // одиночный '!' — лексическая ошибка :)
        outLex = "!"; return T_ERR;
    case '<':
        getChar();
        if (peek() == '<') { getChar(); outLex = "<<"; return SHL; }
        if (peek() == '=') { getChar(); outLex = "<="; return LE; }
        outLex = "<"; return LT;
    case '>':
        getChar();
        if (peek() == '>') { getChar(); outLex = ">>"; return SHR; }
        if (peek() == '=') { getChar(); outLex = ">="; return GE; }
        outLex = ">"; return GT;
    case '/':
        getChar();
        outLex = "/";
        return DIV;
    default:
        // неизвестный/недопустимый символ
    {
        string s;
        s.push_back(getChar());
        outLex = s;
        return T_ERR;
    }
    }
}

std::pair<int, int> Scanner::getLineCol() const {
    int line = 1;
    int col = 0;
    size_t pos = 0;
    while (pos < currentPos && pos < text.size()) {
        char c = text[pos];
        if (c == '\n') {
            line++;
            col = 1;
        }
        else {
            col++;
        }
        pos++;
    }
    return { line, col };
}

size_t Scanner::getPos() const {
    return currentPos;
}

void Scanner::setPos(size_t pos) {
    currentPos = pos;
}

void Scanner::loadFromString(const string& source) {
    text = source;
    text.push_back('\0');
    currentPos = 0;
}