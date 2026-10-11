#include "test_framework.h"
#include "../CompilerC++/Scanner.h"
#include "../CompilerC++/Defines.h"

#include <string>

using std::string;

TEST(Scanner_RecognizesKeywordInt) {
    Scanner sc;
    sc.loadFromString("int");
    string lex;
    CHECK_EQ(sc.getNextLex(lex), KW_INT);
    CHECK_EQ(lex, string("int"));
}

TEST(Scanner_RecognizesAllKeywords) {
    Scanner sc;
    sc.loadFromString("short long bool void switch case default break true false main");
    string lex;
    CHECK_EQ(sc.getNextLex(lex), KW_SHORT);
    CHECK_EQ(sc.getNextLex(lex), KW_LONG);
    CHECK_EQ(sc.getNextLex(lex), KW_BOOL);
    CHECK_EQ(sc.getNextLex(lex), KW_VOID);
    CHECK_EQ(sc.getNextLex(lex), KW_SWITCH);
    CHECK_EQ(sc.getNextLex(lex), KW_CASE);
    CHECK_EQ(sc.getNextLex(lex), KW_DEFAULT);
    CHECK_EQ(sc.getNextLex(lex), KW_BREAK);
    CHECK_EQ(sc.getNextLex(lex), KW_TRUE);
    CHECK_EQ(sc.getNextLex(lex), KW_FALSE);
    CHECK_EQ(sc.getNextLex(lex), KW_MAIN);
}

TEST(Scanner_IdentifierStartingWithKeywordPrefix) {
    Scanner sc;
    sc.loadFromString("integer");
    string lex;
    CHECK_EQ(sc.getNextLex(lex), IDENT);
    CHECK_EQ(lex, string("integer"));
}

TEST(Scanner_Identifier) {
    Scanner sc;
    sc.loadFromString("myVar_123");
    string lex;
    CHECK_EQ(sc.getNextLex(lex), IDENT);
    CHECK_EQ(lex, string("myVar_123"));
}

TEST(Scanner_DecimalConstant) {
    Scanner sc;
    sc.loadFromString("12345");
    string lex;
    CHECK_EQ(sc.getNextLex(lex), DEC_CONST);
    CHECK_EQ(lex, string("12345"));
}

TEST(Scanner_HexConstant) {
    Scanner sc;
    sc.loadFromString("0xFF");
    string lex;
    CHECK_EQ(sc.getNextLex(lex), HEX_CONST);
    CHECK_EQ(lex, string("0xFF"));
}

TEST(Scanner_SingleZeroConstant) {
    Scanner sc;
    sc.loadFromString("0");
    string lex;
    CHECK_EQ(sc.getNextLex(lex), DEC_CONST);
    CHECK_EQ(lex, string("0"));
}

TEST(Scanner_AllOperatorsAndDelimiters) {
    Scanner sc;
    sc.loadFromString("+ - * / % == != < <= > >= << >> = ; , ( ) { } :");
    string lex;
    CHECK_EQ(sc.getNextLex(lex), PLUS);
    CHECK_EQ(sc.getNextLex(lex), MINUS);
    CHECK_EQ(sc.getNextLex(lex), MULT);
    CHECK_EQ(sc.getNextLex(lex), DIV);
    CHECK_EQ(sc.getNextLex(lex), MOD);
    CHECK_EQ(sc.getNextLex(lex), EQ);
    CHECK_EQ(sc.getNextLex(lex), NEQ);
    CHECK_EQ(sc.getNextLex(lex), LT);
    CHECK_EQ(sc.getNextLex(lex), LE);
    CHECK_EQ(sc.getNextLex(lex), GT);
    CHECK_EQ(sc.getNextLex(lex), GE);
    CHECK_EQ(sc.getNextLex(lex), SHL);
    CHECK_EQ(sc.getNextLex(lex), SHR);
    CHECK_EQ(sc.getNextLex(lex), ASSIGN);
    CHECK_EQ(sc.getNextLex(lex), SEMI);
    CHECK_EQ(sc.getNextLex(lex), COMMA);
    CHECK_EQ(sc.getNextLex(lex), LPAREN);
    CHECK_EQ(sc.getNextLex(lex), RPAREN);
    CHECK_EQ(sc.getNextLex(lex), LBRACE);
    CHECK_EQ(sc.getNextLex(lex), RBRACE);
    CHECK_EQ(sc.getNextLex(lex), COLON);
}

TEST(Scanner_SkipsComments) {
    Scanner sc;
    sc.loadFromString("// line comment\nint a /* block\n comment */ = 5");
    string lex;
    CHECK_EQ(sc.getNextLex(lex), KW_INT);
    CHECK_EQ(sc.getNextLex(lex), IDENT);
    CHECK_EQ(lex, string("a"));
    CHECK_EQ(sc.getNextLex(lex), ASSIGN);
    CHECK_EQ(sc.getNextLex(lex), DEC_CONST);
    CHECK_EQ(lex, string("5"));
}

TEST(Scanner_EndOfInput) {
    Scanner sc;
    sc.loadFromString("int");
    string lex;
    sc.getNextLex(lex);
    CHECK_EQ(sc.getNextLex(lex), T_END);
    CHECK_EQ(sc.getNextLex(lex), T_END);
}

TEST(Scanner_IllegalCharacter) {
    Scanner sc;
    sc.loadFromString("$");
    string lex;
    CHECK_EQ(sc.getNextLex(lex), T_ERR);
}

TEST(Scanner_MalformedHexConstant) {
    Scanner sc;
    sc.loadFromString("0x");
    string lex;
    CHECK_EQ(sc.getNextLex(lex), T_ERR);
}

TEST(Scanner_LineTracking) {
    Scanner sc;
    sc.loadFromString("a\nbb");
    string lex;
    sc.getNextLex(lex);
    CHECK_EQ(lex, string("a"));
    sc.getNextLex(lex);
    CHECK_EQ(lex, string("bb"));
    CHECK_EQ(sc.getLineCol().first, 2);
}

TEST(Scanner_PositionSaveAndRestore) {
    Scanner sc;
    sc.loadFromString("int x");
    string lex;
    sc.getNextLex(lex);
    size_t pos = sc.getPos();
    sc.getNextLex(lex);
    CHECK_EQ(lex, string("x"));
    sc.setPos(pos);
    sc.getNextLex(lex);
    CHECK_EQ(lex, string("x"));
}
