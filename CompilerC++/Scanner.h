#pragma once
#include <cstddef>
#include <string>
#include <utility>

using namespace std;

// Lexical analyzer: converts the source text into a stream of tokens.
class Scanner {
public:
    Scanner();

    // Load a file; returns true on success.
    bool loadFile(const string& fileName);
    // Load source text from a string (useful for tests).
    void loadFromString(const std::string& source);

    // Read the next token. The token text is written to outLex and its code is returned.
    int getNextLex(string& outLex);

    // Current (line, column) of the read position, 1-based for both.
    std::pair<int, int> getLineCol() const;

    // Current offset in the internal buffer.
    size_t getPos() const;

    // Set the offset; the next read starts from this position.
    void setPos(size_t pos);

private:
    string text;        // source text with a trailing '\0'
    size_t currentPos;  // current offset in text

    char peek(size_t offset = 0) const; // look at a character without advancing
    char getChar();                     // read a character and advance
    void ungetChar();                   // step one character back

    // Character classifiers
    static bool isLetter(char c);
    static bool isDigit(char c);
    static bool isHexDigit(char c);
    static bool isIdentStart(char c);
    static bool isIdentPart(char c);

    void skipIgnored();                  // skip whitespace and comments
    int checkKeyword(const string& s);   // map an identifier to a keyword code
};
