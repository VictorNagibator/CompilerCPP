#include "scanner.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Простые конструктор и деструктор
Scanner::Scanner() : text(), uk(0) {}
Scanner::~Scanner() {}

bool Scanner::loadFile(const string& fileName) {
    ifstream in(fileName);
    if (!in) return false;

    ostringstream ss;
    ss << in.rdbuf();
    text = ss.str();

    // Добавляем нулевой символ гарантированно
    text.push_back('\0');
    uk = 0;
    return true;
}

char Scanner::peek(size_t offset) const {
    size_t pos = uk + offset;
    if (pos < text.size()) return text[pos];
    return '\0';
}
char Scanner::getChar() {
    if (uk < text.size()) return text[uk++];
    return '\0';
}
void Scanner::ungetChar() {
    if (uk > 0) --uk;
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
                // однострочный комментарий: consume '//' и читать до LF или EOF
                getChar(); getChar();
                while (peek() != '\n' && peek() != '\0') getChar();
                // оставим newline на следующем проходе
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

// Основной метод: получить следующую лексему
int Scanner::getNextToken(string& outLex) {
    outLex.clear();

	//Пропускаем игнорируемые символы
    skipIgnored();

    char c = peek();
    if (c == '\0') { return T_END; }

    // Идентификатор или ключевое слово
    if (isIdentStart(c)) {
        string lex;
        lex.push_back(getChar());
        while (isIdentPart(peek())) lex.push_back(getChar());
        int code = checkKeyword(lex);
        outLex = lex;
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
                return HEX_CONST;
            }
            else if (isDigit(nx)) {
                // цифра после 0 - читаем как DEC
                string lex;
                lex.push_back(first);
                while (isDigit(peek())) lex.push_back(getChar());
                outLex = lex;
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