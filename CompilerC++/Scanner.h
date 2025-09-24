#include <string>

using namespace std;

class Scanner {
public:
    Scanner();
    ~Scanner();

    // Загрузить файл; вернуть true при успехе
    bool loadFile(const string& fileName);

    // Получить следующую лексему; лексема записывается в outLex
    // Возвращает её код 
    int getNextLex(string& outLex);

private:
    string text; // исходный текст + завершающий '\0'
    size_t uk; // текущая позиция в text

	char peek(size_t offset = 0) const; // получить текущий символ, но не сдвигать позицию
	char getChar(); // получить текущий символ и сдвинуть позицию
	void ungetChar(); // "вернуть назад" последний полученный символ (сдвинуть позицию влево)

	// классификаторы символов (буква, цифра, начало идентификатора и т.п.)
    static bool isLetter(char c);
    static bool isDigit(char c);
    static bool isHexDigit(char c);
    static bool isIdentStart(char c);
    static bool isIdentPart(char c);

    void skipIgnored(); // проверить пробелы и комментарии
    int checkKeyword(const string& s); // проверить ключевые слова
};