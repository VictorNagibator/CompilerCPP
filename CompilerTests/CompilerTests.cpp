#include "pch.h"                   
#include "CppUnitTest.h"   

#include "../CompilerC++/Scanner.cpp" // Реализация лексера
#include "../CompilerC++/Tree.cpp" // Реализация семантического дерева
#include "../CompilerC++/DataType.h" // Типы данных
#include "../CompilerC++/Defines.h" // Коды лексем

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

namespace UnitTests
{
    // Сброс глобального состояния дерева (Tree).
    // Поскольку Tree содержит статические поля (Root, Cur и флаги), необходимо
    // перед каждым тестом, который их использует, приводить дерево в исходное состояние,
    // чтобы тесты не влияли друг на друга
    void ResetTree()
    {
        Tree::reset(); // Метод, обнуляющий всё дерево

        // После сброса создаём корневую область видимости
        SemNode* rootNode = new SemNode();
        rootNode->id = "root";
        rootNode->DataType = TYPE_SCOPE;
        Tree::Root = new Tree(rootNode, nullptr);
        Tree::Cur = Tree::Root;
    }

    // Тесты лексера
    TEST_CLASS(ScannerTests)
    {
    public:
        // 1. Проверка распознавания ключевого слова int
        TEST_METHOD(TestKeywordInt)
        {
            Scanner sc;
            sc.loadFromString("int"); // Загружаем строку
            string lex;
            int tok = sc.getNextLex(lex);

            Assert::AreEqual(KW_INT, tok); // Ожидаем код ключевого слова
        }

        // 2. Проверка идентификатора (имя переменной/функции)
        TEST_METHOD(TestIdentifier)
        {
            Scanner sc;
            sc.loadFromString("myVar123");
            string lex;
            int tok = sc.getNextLex(lex);

            Assert::AreEqual(IDENT, tok);
        }

        // 3. Десятичная константа
        TEST_METHOD(TestDecConstant)
        {
            Scanner sc;
            sc.loadFromString("12345");
            string lex;
            int tok = sc.getNextLex(lex);

            Assert::AreEqual(DEC_CONST, tok);
        }

        // 4. Шестнадцатеричная константа (начинается с 0x)
        TEST_METHOD(TestHexConstant)
        {
            Scanner sc;
            sc.loadFromString("0xFF");
            string lex;
            int tok = sc.getNextLex(lex);

            Assert::AreEqual(HEX_CONST, tok);
        }

        // 5. Проверка пропуска однострочных (//) и многострочных (/* */) комментариев
        TEST_METHOD(TestSkipComments)
        {
            Scanner sc;
            sc.loadFromString("// comment\nint a/* block */=5");
            string lex;
            int tok;

            tok = sc.getNextLex(lex); Assert::AreEqual(KW_INT, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(IDENT, tok); Assert::AreEqual(string("a"), lex);
            tok = sc.getNextLex(lex); Assert::AreEqual(ASSIGN, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(DEC_CONST, tok); Assert::AreEqual(string("5"), lex);
        }

        // 6. Все операторы и разделители
        TEST_METHOD(TestOperators)
        {
            Scanner sc;
            sc.loadFromString("+ - * / % == != < <= > >= << >> = ; , ( ) { } :");
            string lex;
            int tok;

            tok = sc.getNextLex(lex); Assert::AreEqual(PLUS, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(MINUS, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(MULT, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(DIV, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(MOD, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(EQ, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(NEQ, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(LT, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(LE, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(GT, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(GE, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(SHL, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(SHR, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(ASSIGN, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(SEMI, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(COMMA, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(LPAREN, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(RPAREN, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(LBRACE, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(RBRACE, tok);
            tok = sc.getNextLex(lex); Assert::AreEqual(COLON, tok);
        }
    };

    // Тесты семантических операций
    TEST_CLASS(SemanticTests)
    {
    public:
        // Перед каждым тестом сбрасываем дерево в исходное состояние
        TEST_METHOD_INITIALIZE(Init)
        {
            ResetTree();
        }

        // 7. Добавление переменной в текущую область видимости и проверка, что она там есть
        TEST_METHOD(TestIncludeVariable)
        {
            Tree::Cur->semInclude("x", TYPE_INT, 1, 1);
            Tree* found = Tree::Cur->findUp(Tree::Cur, "x");

            Assert::IsNotNull(found);
            Assert::AreEqual(string("x"), found->n->id);
            Assert::AreEqual((int)TYPE_INT, (int)found->n->DataType); // Enum требуется перевести в int - иначе не работает Assert :(
        }

        // 8. Попытка повторного объявления в той же области
        TEST_METHOD(TestDuplicateVariable)
        {
            Tree::Cur->semInclude("x", TYPE_INT, 1, 1);
            bool dup = Tree::Cur->dupControl(Tree::Cur, "x");

            Assert::IsTrue(dup); // dupControl должен вернуть true (повторное использование переменной!)
        }

        // 9. Поиск переменной в родительской области
        TEST_METHOD(TestFindVariableInParent)
        {
            // Объявляем x в глобальной области
            Tree::Cur->semInclude("x", TYPE_INT, 1, 1);
            // Входим во вложенный блок
            Tree::Cur->semEnterBlock(2, 1);
            // Ищем x из блока — он должен найтись в родителе
            Tree* found = Tree::Cur->findUp(Tree::Cur, "x");

            Assert::IsNotNull(found);
            Assert::AreEqual(string("x"), found->n->id);

            Tree::Cur->semExitBlock(); // Возвращаемся в глобальную область
        }

        // 10. Присваивание значения переменной и последующее чтение
        TEST_METHOD(TestSetVarValue)
        {
            Tree::Cur->semInclude("y", TYPE_INT, 1, 1);
            SemNode val;
            val.DataType = TYPE_INT;
            val.hasValue = true;
            val.Value.v_int32 = 42;

            Tree::setVarValue("y", val, 1, 1);
            SemNode retrieved = Tree::getVarValue("y", 1, 1);

            Assert::IsTrue(retrieved.hasValue); // Есть ли значение
            Assert::AreEqual(42, retrieved.Value.v_int32); // Равно ли тому, что мы присвоили
        }

        // 11. Повторное объявление переменной с тем же именем в разных блоках
        TEST_METHOD(TestDupDifferentScope)
        {
            // Объявляем x во внешнем блоке
            Tree::Cur->semInclude("x", TYPE_INT, 1, 1);
            // Входим во внутренний блок
            Tree* block = Tree::Cur->semEnterBlock(2, 1);
            // Во внутреннем блоке ещё нет x - dupControl должен вернуть false
            bool dup = Tree::Cur->dupControl(Tree::Cur, "x");
            Assert::IsFalse(dup);

            // Теперь объявляем x во внутреннем блоке (допустимо)
            Tree::Cur->semInclude("x", TYPE_BOOL, 2, 2);
            // Проверяем, что теперь повторное объявление есть уже во внутреннем блоке
            dup = Tree::Cur->dupControl(Tree::Cur, "x");
            Assert::IsTrue(dup);
        }
    };

    // Тесты вычисления выражений (арифметика, сравнения, сдвиги)
    TEST_CLASS(ExpressionTests)
    {
    public:
        // 12. Арифметическая операция сложения двух целых
        TEST_METHOD(TestArithmeticAdd)
        {
            SemNode left, right;
            // Два целых числа: 5 + 3
            left.DataType = TYPE_INT; left.hasValue = true; left.Value.v_int32 = 5;
            right.DataType = TYPE_INT; right.hasValue = true; right.Value.v_int32 = 3;

            SemNode result = Tree::executeArithmeticOp(left, right, "+", 1, 1);

            // Должно получиться такое же целое 8
            Assert::IsTrue(result.hasValue);
            Assert::AreEqual(8, result.Value.v_int32); 
            Assert::AreEqual((int)TYPE_INT, (int)result.DataType);
        }

        // 13. Операция сравнения < (результат bool)
        TEST_METHOD(TestComparisonLess)
        {
            // 5 < 10
            SemNode left, right;
            left.DataType = TYPE_INT; left.hasValue = true; left.Value.v_int32 = 5;
            right.DataType = TYPE_INT; right.hasValue = true; right.Value.v_int32 = 10;

            SemNode result = Tree::executeComparisonOp(left, right, "<", 1, 1);

            // Должно вернуть true
            Assert::IsTrue(result.hasValue);
            Assert::IsTrue(result.Value.v_bool);
            Assert::AreEqual((int)TYPE_BOOL, (int)result.DataType);
        }

        // 14. Операция сдвига влево (побитовый сдвиг)
        TEST_METHOD(TestShiftLeft)
        {
            SemNode left, right;
            left.DataType = TYPE_INT; left.hasValue = true; left.Value.v_int32 = 4; // двоичное 100
            right.DataType = TYPE_INT; right.hasValue = true; right.Value.v_int32 = 2;

            SemNode result = Tree::executeShiftOp(left, right, "<<", 1, 1);

            Assert::IsTrue(result.hasValue);
            Assert::AreEqual(16, result.Value.v_int32); // 4 << 2 = 16 (100 << 2 = 10000)
        }
    };
}