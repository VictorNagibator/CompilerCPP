#include "Scanner.h"
#include "Defines.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Maximum length of a numeric literal and of an identifier.
#define MAX_CONST_LEN 20
#define MAX_IDENT_LEN 20

Scanner::Scanner() : text(), currentPos(0) {}

bool Scanner::loadFile(const string& fileName) {
    ifstream in(fileName);
    if (!in) return false;

    ostringstream ss;
    ss << in.rdbuf();
    text = ss.str();

    // Append a sentinel so peek()/getChar() can stop at the end of the buffer.
    text.push_back('\0');
    currentPos = 0;
    return true;
}

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

// Character classifiers
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

// Map an identifier to the corresponding keyword code, or IDENT otherwise.
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
    if (s == "true") return KW_TRUE;   // boolean literals are keywords
    if (s == "false") return KW_FALSE;
    if (s == "main") return KW_MAIN;
    return IDENT;
}

// Skip whitespace and comments (both // and /* ... */).
void Scanner::skipIgnored() {
    for (;;) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            getChar();
            continue;
        }
        if (c == '/') {
            char n = peek(1);
            if (n == '/') {
                // Single-line comment: drop '//' and read until the end of the line.
                getChar(); getChar();
                while (peek() != '\n' && peek() != '\0') getChar();
                continue;
            }
            else if (n == '*') {
                // Block comment /* ... */
                getChar(); getChar(); // drop '/*'
                while (peek() != '\0') {
                    char ch = getChar();
                    if (ch == '*' && peek() == '/') {
                        getChar(); // drop '/'
                        break;
                    }
                }
                continue;
            }
        }
        break;
    }
}

// Read the next token from the source.
int Scanner::getNextLex(string& outLex) {
    outLex.clear();

    skipIgnored();

    char c = peek();
    if (c == '\0') { return T_END; }

    // Identifier or keyword
    if (isIdentStart(c)) {
        string lex;
        lex.push_back(getChar());
        while (isIdentPart(peek())) lex.push_back(getChar());
        int code = checkKeyword(lex);
        outLex = lex;

        if (code == IDENT && lex.length() > MAX_IDENT_LEN) return T_ERR;

        return code;
    }

    // Decimal and hexadecimal constants
    if (isDigit(c)) {
        char first = getChar();
        if (first == '0') {
            char nx = peek();
            if (nx == 'x' || nx == 'X') {
                getChar(); // drop x/X
                string lex = "0";
                lex.push_back(nx);
                if (!isHexDigit(peek())) {
                    outLex = lex;
                    return T_ERR;
                }
                while (isHexDigit(peek())) lex.push_back(getChar());
                outLex = lex;

                if (lex.length() > MAX_CONST_LEN) return T_ERR;

                return HEX_CONST;
            }
            else if (isDigit(nx)) {
                // A digit after the leading 0: read as a decimal constant.
                string lex;
                lex.push_back(first);
                while (isDigit(peek())) lex.push_back(getChar());
                outLex = lex;

                if (lex.length() > MAX_CONST_LEN) return T_ERR;

                return DEC_CONST;
            }
            else {
                outLex = "0";
                return DEC_CONST;
            }
        }
        else {
            string lex;
            lex.push_back(first);
            while (isDigit(peek())) lex.push_back(getChar());
            outLex = lex;

            if (lex.length() > MAX_CONST_LEN) return T_ERR;

            return DEC_CONST;
        }
    }

    // Operators and delimiters
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
        outLex = "!"; return T_ERR; // a lone '!' is a lexical error
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
        // Unknown/invalid character
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
