#include <string>

using namespace std;

// ���� ������� (� ������������ � �������� �� ������ 3) 
#define KW_INT     1
#define KW_SHORT   2
#define KW_LONG    3
#define KW_BOOL    4
#define KW_VOID    5
#define KW_SWITCH  6
#define KW_CASE    7
#define KW_DEFAULT 8
#define KW_BREAK   9
#define KW_TRUE    10
#define KW_FALSE   11

#define IDENT      20

#define DEC_CONST  30
#define HEX_CONST  31
#define BOOL_CONST 32 

#define SEMI       40
#define COMMA      41
#define LPAREN     42
#define RPAREN     43
#define LBRACE     44
#define RBRACE     45
#define COLON      46

#define EQ         50  
#define NEQ        51 
#define LE         52
#define GE         53  
#define SHL        54  
#define SHR        55  
#define LT         56 
#define GT         57 
#define ASSIGN     58 
#define PLUS       59
#define MINUS      60
#define MULT       61
#define DIV        62
#define MOD        63

#define T_END     100
#define T_ERR     200

class Scanner {
public:
    Scanner();
    ~Scanner();

    // ��������� ����; ������� true ��� ������
    bool loadFile(const string& fileName);

    // �������� ��������� �����; ������� ������������ � outLex
    // ���������� ��� ������
    int getNextToken(string& outLex);

private:
    string text; // �������� ����� + ����������� '\0'
    size_t uk; // ������� ������� � text

	char peek(size_t offset = 0) const; // �������� ������� ������, �� �� �������� �������
	char getChar(); // �������� ������� ������ � �������� �������
	void ungetChar(); // "������� �����" ��������� ���������� ������ (�������� ������� �����)

	// �������������� �������� (�����, �����, ������ �������������� � �.�.)
    static bool isLetter(char c);
    static bool isDigit(char c);
    static bool isHexDigit(char c);
    static bool isIdentStart(char c);
    static bool isIdentPart(char c);

    void skipIgnored(); // ��������� ������� � �����������
    int checkKeyword(const string& s); // ��������� �������� �����
};