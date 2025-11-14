#include "Tree.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <functional>

// Инициализация статических полей
Tree* Tree::Root = nullptr;
Tree* Tree::Cur = nullptr;
bool Tree::interpretationEnabled = true; // По умолчанию включена
bool Tree::debug = true; // По умолчанию включен подробный вывод
Tree* Tree::currentFunction = nullptr; 
int Tree::recursionDepth = 0;

void Tree::enterFunctionCall(const string& funcName, int line, int col) {
    recursionDepth++;
    if (recursionDepth > MAX_RECURSION_DEPTH) {
        interpError("превышение глубины рекурсии", funcName, line, col);
    }
}

void Tree::exitFunctionCall() {
    if (recursionDepth > 0) recursionDepth--;
}

void Tree::printTypeConversionWarning(DATA_TYPE from, DATA_TYPE to, const string& context,
    const string& expression, int line, int col) {

    // Выводим только если включен debug режим
    if (!debug || !interpretationEnabled) return;

    std::cerr << "Предупреждение: неявное преобразование типа ";

    switch (from) {
        case TYPE_SHORT_INT: std::cerr << "short"; break;
        case TYPE_INT: std::cerr << "int"; break;
        case TYPE_LONG_INT: std::cerr << "long"; break;
        case TYPE_BOOL: std::cerr << "bool"; break;
        default: std::cerr << "unknown"; break;
    }

    std::cerr << " к ";

    switch (to) {
        case TYPE_SHORT_INT: std::cerr << "short"; break;
        case TYPE_INT: std::cerr << "int"; break;
        case TYPE_LONG_INT: std::cerr << "long"; break;
        case TYPE_BOOL: std::cerr << "bool"; break;
        default: std::cerr << "unknown"; break;
    }

    std::cerr << " в " << context;
    if (!expression.empty()) {
        std::cerr << " выражения " << expression;
    }
    std::cerr << std::endl << "(строка " << line << ":" << col << ")" << std::endl;
}

// печать семантической ошибки
void Tree::semError(const string& msg, const string& id, int line, int col) {
    std::cerr << "Семантическая ошибка: " << msg;
    if (!id.empty()) std::cerr << " (около '" << id << "')";
    std::cerr << std::endl << "(строка " << line << ":" << col << ")" << std::endl;
    std::exit(1);
}

void Tree::interpError(const string& msg, const string& id, int line, int col) {
    std::cerr << "Ошибка при интерпретации: " << msg;
    if (!id.empty()) std::cerr << " (около '" << id << "')";
    std::cerr << std::endl << "(строка " << line << ":" << col << ")" << std::endl;
    std::exit(1);
}

// Конструктор
Tree::Tree(SemNode* node, Tree* up) : n(node), Up(up), Left(nullptr), Right(nullptr) {
    if (Root == nullptr) {
        Root = this;
        Cur = this;
    }
}

// Деструктор: рекурсивно удаляем поддерево
Tree::~Tree() {
    if (n) delete n;
    if (Left) { delete Left; Left = nullptr; }
    if (Right) { delete Right; Right = nullptr; }
}

// Вставка первого дочернего элемента (левая ссылка)
void Tree::setLeft(SemNode* Data) {
    Tree* newNode = new Tree(Data, this);
    if (this->Left == nullptr) {
        this->Left = newNode;
    }
    else {
        Tree* p = this->Left;
        while (p->Right) p = p->Right;
        p->Right = newNode;
        newNode->Up = this;
    }
}

// Вставка правого соседа
void Tree::setRight(SemNode* Data) {
    Tree* newNode = new Tree(Data, this->Up);
    newNode->Right = this->Right;
    this->Right = newNode;
    newNode->Up = this->Up;
}

// ищет имя id среди дочерних элементов узла
Tree* Tree::findUpOneLevel(Tree* From, const string& id) {
    if (From == nullptr) return nullptr;
    Tree* p = From->Left;
    while (p != nullptr) {
        if (p->n && p->n->id == id) return p;
        p = p->Right;
    }
    return nullptr;
}

// поиск с подъёмом по областям (для блочной видимости)
Tree* Tree::findUp(Tree* From, const string& id) {
    Tree* cur = From;
    while (cur != nullptr) {
        Tree* found = findUpOneLevel(cur, id);
        if (found) return found;
        cur = cur->Up;
    }
    return nullptr;
}

// проверка дубля на уровне Addr (Addr — текущая область)
bool Tree::dupControl(Tree* Addr, const string& a) {
    return findUpOneLevel(Addr, a) != nullptr;
}

// добавляет идентификатор в текущую область Cur
Tree* Tree::semInclude(const string& a, DATA_TYPE t, int line, int col) {
    if (Cur == nullptr) {
        semError("внутренняя ошибка: текущая область не установлена при SemInclude", a, line, col);
    }

    if (dupControl(Cur, a)) {
        semError("повторное описание идентификатора", a, line, col);
    }

    SemNode* node = new SemNode();
    node->id = a;
    node->DataType = t;
    node->hasValue = false;
    node->Param = 0;
    node->line = line;
    node->col = col;

    if (t == TYPE_FUNCT) {
        Cur->setLeft(node);
        Tree* funcNode = Cur->Left;
        while (funcNode->Right) funcNode = funcNode->Right;

        SemNode* emptyNode = new SemNode();
        emptyNode->id = "";
        emptyNode->DataType = TYPE_SCOPE;
        emptyNode->Param = 0;
        emptyNode->line = line;
        emptyNode->col = col;

        // ВАЖНО: scope должен быть child (Left) функции, а не её "правым соседом".
        funcNode->setLeft(emptyNode);

        // Вернуть указатель на функцию (как и раньше)
        return funcNode;
    }
    else {
        Cur->setLeft(node);
        Tree* added = Cur->Left;
        while (added->Right) added = added->Right;

        return added;
    }
}

// занесение константы со значением
Tree* Tree::semIncludeConstant(const string& a, DATA_TYPE t, const string& value, int line, int col) {
    Tree* node = semInclude(a, t, line, col);
    if (node && node->n) {
        node->n->hasValue = true;
        // Преобразование строкового значения в соответствующий тип
        try {
            if (t == TYPE_BOOL) {
                node->n->Value.v_bool = (value == "true");
            }
            else {
                long long val = std::stoll(value, nullptr, 0);
                if (t == TYPE_SHORT_INT) {
                    node->n->Value.v_int16 = static_cast<int16_t>(val);
                }
                else if (t == TYPE_INT) {
                    node->n->Value.v_int32 = static_cast<int32_t>(val);
                }
                else if (t == TYPE_LONG_INT) {
                    node->n->Value.v_int64 = static_cast<int64_t>(val);
                }
            }
        }
        catch (const std::exception& e) {
            semError("неверный формат константы: " + string(e.what()), a, line, col);
        }
    }
    return node;
}

void Tree::semSetParam(Tree* Addr, int n) {
    if (Addr == nullptr || Addr->n == nullptr) {
        semError("SemSetParam: неверный адрес функции");
    }
    Addr->n->Param = n;
}

void Tree::semSetParamTypes(Tree* Addr, const std::vector<DATA_TYPE>& types) {
    if (Addr == nullptr || Addr->n == nullptr) {
        semError("SemSetParamTypes: неверный адрес функции");
    }
    Addr->n->ParamTypes = types;
    Addr->n->Param = static_cast<int>(types.size());
}

void Tree::semControlParamTypes(Tree* Addr, const std::vector<DATA_TYPE>& argTypes, int line, int col) {
    if (Addr == nullptr || Addr->n == nullptr) {
        semError("SemControlParamTypes: неверный адрес функции");
    }
    const std::vector<DATA_TYPE>& formal = Addr->n->ParamTypes;
    if (formal.size() != argTypes.size()) {
        semError("неверное число параметров у функции", Addr->n->id, line, col);
    }
    for (size_t i = 0; i < formal.size(); ++i) {
        bool formalIsInt = (formal[i] == TYPE_INT || formal[i] == TYPE_SHORT_INT || formal[i] == TYPE_LONG_INT);
        bool argIsInt = (argTypes[i] == TYPE_INT || argTypes[i] == TYPE_SHORT_INT || argTypes[i] == TYPE_LONG_INT);
        if (formalIsInt && argIsInt) continue;
        if (formal[i] == argTypes[i]) continue;
        semError("несоответствие типов параметров у функции", Addr->n->id, line, col);
    }
}

Tree* Tree::semGetVar(const string& a, int line, int col) {
    Tree* v = findUp(Cur, a);
    if (v == nullptr) {
        semError("отсутствует описание идентификатора", a, line, col);
    }
    if (v->n->DataType == TYPE_FUNCT) {
        semError("неверное использование - идентификатор является функцией", a, line, col);
    }
    return v;
}

Tree* Tree::semGetFunct(const string& a, int line, int col) {
    Tree* v = findUp(Cur, a);
    if (v == nullptr) {
        semError("отсутствует описание функции", a, line, col);
    }
    if (v->n->DataType != TYPE_FUNCT) {
        semError("идентификатор не является функцией", a, line, col);
    }
    return v;
}

Tree* Tree::semEnterBlock(int line, int col) {
    if (Cur == nullptr) {
        semError("SemEnterBlock: текущая область не установлена");
    }
    SemNode* sn = new SemNode();
    sn->id = "";
    sn->DataType = TYPE_SCOPE;
    sn->Param = 0;
    sn->line = line;
    sn->col = col;

    setLeft(sn);
    Tree* created = Cur->Left;
    while (created->Right) created = created->Right;

    Cur = created;
    return created;
}

void Tree::semExitBlock() {
    if (Cur == nullptr) {
        semError("SemExitBlock: текущая область не установлена");
    }
    if (Cur->Up == nullptr) {
        semError("SemExitBlock: попытка выйти из корневой области");
    }
    Cur = Cur->Up;
}

void Tree::setVarValue(const string& name, const SemNode& value, int line, int col) {
    Tree* varNode = Cur->semGetVar(name, line, col);

    if (!value.hasValue) {
        interpError("попытка присвоить NULL", name, line, col);
    }

    // Проверка совместимости типов
    if (!canImplicitCast(value.DataType, varNode->n->DataType)) {
        semError("несовместимые типы при присваивании", name, line, col);
    }

    // Проверяем обрезку значений для ВСЕХ типов (как было)
    bool needsTruncationWarning = false;
    long long originalValue = 0;
    switch (value.DataType) {
    case TYPE_SHORT_INT: originalValue = value.Value.v_int16; break;
    case TYPE_INT:       originalValue = value.Value.v_int32; break;
    case TYPE_LONG_INT:  originalValue = value.Value.v_int64; break;
    default: break;
    }

    if (varNode->n->DataType == TYPE_SHORT_INT) {
        if (originalValue < -32768 || originalValue > 32767) needsTruncationWarning = true;
    }
    else if (varNode->n->DataType == TYPE_INT) {
        if (originalValue < -2147483648LL || originalValue > 2147483647LL) needsTruncationWarning = true;
    }

    if (needsTruncationWarning) {
        std::cerr << "Предупреждение: значение " << originalValue
            << " обрезается при преобразовании к "
            << (varNode->n->DataType == TYPE_SHORT_INT ? "short" : "int");
        std::cerr << std::endl << "(строка " << line << ":" << col << ")" << std::endl;
    }
    else if (value.DataType != varNode->n->DataType && debug) {
        printTypeConversionWarning(value.DataType, varNode->n->DataType,
            "присваивании", name + " = ...", line, col);
    }

    // Выполняем приведение значения к типу переменной
    SemNode converted = castToType(value, varNode->n->DataType, line, col);

    // Если интерпретация выключена — мы в семантическом режиме: не меняем runtime-значение в дереве,
    // но пометим переменную как инициализированную (чтобы дальнейшая семантика в том же блоке работала)
    if (!Tree::isInterpretationEnabled()) {
        // Пометка "инициализирована" для семантики
        varNode->n->hasValue = true;
        return;
    }

    // Нормальное (runtime) присваивание — только если интерпретация включена
    varNode->n->Value = converted.Value;
    varNode->n->hasValue = true;

    printAssignment(name, converted, line, col);
}

SemNode Tree::getVarValue(const string& name, int line, int col) {
    Tree* varNode = Cur->semGetVar(name, line, col); // Используем Cur->
    if (!varNode->n->hasValue) {
        semError("использование неинициализированной переменной", name, line, col);
    }
    return *(varNode->n);
}

void Tree::executeFunctionCall(const string& funcName, const std::vector<SemNode>& args, int line, int col) {
    Tree* funcNode = Cur->semGetFunct(funcName, line, col);

    std::vector<DATA_TYPE> argTypes;
    for (const auto& arg : args) {
        argTypes.push_back(arg.DataType);
    }

    funcNode->semControlParamTypes(funcNode, argTypes, line, col);

    // Используем новый метод вывода
    printFunctionCall(funcName, args, line, col);
}

// Определение максимального типа
DATA_TYPE Tree::getMaxType(DATA_TYPE t1, DATA_TYPE t2) {
    if (t1 == TYPE_LONG_INT || t2 == TYPE_LONG_INT) return TYPE_LONG_INT;
    if (t1 == TYPE_INT || t2 == TYPE_INT) return TYPE_INT;
    return TYPE_SHORT_INT;
}

// Проверка возможности неявного приведения
bool Tree::canImplicitCast(DATA_TYPE from, DATA_TYPE to) {
    // Разрешено между целочисленными типами
    bool fromIsInt = (from == TYPE_SHORT_INT || from == TYPE_INT || from == TYPE_LONG_INT);
    bool toIsInt = (to == TYPE_SHORT_INT || to == TYPE_INT || to == TYPE_LONG_INT);
    return (fromIsInt && toIsInt) || (from == TYPE_BOOL && to == TYPE_BOOL);
}

// Приведение типов
SemNode Tree::castToType(const SemNode& value, DATA_TYPE targetType, int line, int col, bool showWarning) {
    // Если типы уже совпадают, возвращаем без изменений
    if (value.DataType == targetType) {
        return value;
    }

    SemNode result;
    result.DataType = targetType;
    result.hasValue = value.hasValue;

    if (!value.hasValue) return result;

    // Получаем значение в long long для безопасного приведения
    long long originalValue = 0;
        switch (value.DataType) {
        case TYPE_SHORT_INT: originalValue = value.Value.v_int16; break;
        case TYPE_INT: originalValue = value.Value.v_int32; break;
        case TYPE_LONG_INT: originalValue = value.Value.v_int64; break;
        default: break;
    }

    // Выполняем приведение
    switch (targetType) {
    case TYPE_SHORT_INT:
        result.Value.v_int16 = static_cast<int16_t>(originalValue);
        break;
    case TYPE_INT:
        result.Value.v_int32 = static_cast<int32_t>(originalValue);
        break;
    case TYPE_LONG_INT:
        result.Value.v_int64 = static_cast<int64_t>(originalValue);
        break;
    case TYPE_BOOL:
        if (value.DataType == TYPE_BOOL) {
            result.Value.v_bool = value.Value.v_bool;
        }
        else {
            semError("недопустимое приведение целого типа к bool", "", line, col);
        }
        break;
    default:
        semError("неизвестный тип для приведения", "", line, col);
    }

    return result;
}
// Арифметические операции
SemNode Tree::executeArithmeticOp(const SemNode& left, const SemNode& right, const string& op, int line, int col) {
    if (!left.hasValue || !right.hasValue) {
        semError("операция с неинициализированными значениями", "", line, col);
    }

    // Выводим предупреждение если операнды разных типов
    if (left.DataType != right.DataType && debug) {
        printTypeConversionWarning(left.DataType, right.DataType,
            "арифметической операции", "", line, col);
    }

    DATA_TYPE resultType = getMaxType(left.DataType, right.DataType);
    SemNode leftConv = castToType(left, resultType, line, col);
    SemNode rightConv = castToType(right, resultType, line, col);

    SemNode result;
    result.DataType = resultType;
    result.hasValue = true;

    if (!interpretationEnabled) return result;

    switch (resultType) {
    case TYPE_SHORT_INT:
        if (op == "+") result.Value.v_int16 = leftConv.Value.v_int16 + rightConv.Value.v_int16;
        else if (op == "-") result.Value.v_int16 = leftConv.Value.v_int16 - rightConv.Value.v_int16;
        else if (op == "*") result.Value.v_int16 = leftConv.Value.v_int16 * rightConv.Value.v_int16;
        else if (op == "/") {
            if (rightConv.Value.v_int16 == 0) interpError("деление на ноль", "", line, col);
            result.Value.v_int16 = leftConv.Value.v_int16 / rightConv.Value.v_int16;
        }
        else if (op == "%") {
            if (rightConv.Value.v_int16 == 0) interpError("деление на ноль", "", line, col);
            result.Value.v_int16 = leftConv.Value.v_int16 % rightConv.Value.v_int16;
        }
        break;

    case TYPE_INT:
        if (op == "+") result.Value.v_int32 = leftConv.Value.v_int32 + rightConv.Value.v_int32;
        else if (op == "-") result.Value.v_int32 = leftConv.Value.v_int32 - rightConv.Value.v_int32;
        else if (op == "*") result.Value.v_int32 = leftConv.Value.v_int32 * rightConv.Value.v_int32;
        else if (op == "/") {
            if (rightConv.Value.v_int32 == 0) interpError("деление на ноль", "", line, col);
            result.Value.v_int32 = leftConv.Value.v_int32 / rightConv.Value.v_int32;
        }
        else if (op == "%") {
            if (rightConv.Value.v_int32 == 0) interpError("деление на ноль", "", line, col);
            result.Value.v_int32 = leftConv.Value.v_int32 % rightConv.Value.v_int32;
        }
        break;

    case TYPE_LONG_INT:
        if (op == "+") result.Value.v_int64 = leftConv.Value.v_int64 + rightConv.Value.v_int64;
        else if (op == "-") result.Value.v_int64 = leftConv.Value.v_int64 - rightConv.Value.v_int64;
        else if (op == "*") result.Value.v_int64 = leftConv.Value.v_int64 * rightConv.Value.v_int64;
        else if (op == "/") {
            if (rightConv.Value.v_int64 == 0) interpError("деление на ноль", "", line, col);
            result.Value.v_int64 = leftConv.Value.v_int64 / rightConv.Value.v_int64;
        }
        else if (op == "%") {
            if (rightConv.Value.v_int64 == 0) interpError("деление на ноль", "", line, col);
            result.Value.v_int64 = leftConv.Value.v_int64 % rightConv.Value.v_int64;
        }
        break;

    default:
        semError("неподдерживаемый тип для арифметической операции", "", line, col);
    }

    // Вывод информации об операции (отладочный)
    if (debug && interpretationEnabled) {
        printArithmeticOp(op, leftConv, rightConv, result, line, col);
    }

    return result;
}

// Операции сдвига
SemNode Tree::executeShiftOp(const SemNode& left, const SemNode& right, const string& op, int line, int col) {
    if (!left.hasValue || !right.hasValue) {
        semError("операция с неинициализированными значениями", "", line, col);
    }

    SemNode result;
    result.DataType = left.DataType; // Результат имеет тип левого операнда
    result.hasValue = true;

    if (!interpretationEnabled) return result;

    // Приводим правый операнд к int для сдвига
    SemNode rightConv = castToType(right, TYPE_INT, line, col);

    switch (left.DataType) {
    case TYPE_SHORT_INT:
        if (op == "<<") result.Value.v_int16 = left.Value.v_int16 << rightConv.Value.v_int32;
        else if (op == ">>") result.Value.v_int16 = left.Value.v_int16 >> rightConv.Value.v_int32;
        break;

    case TYPE_INT:
        if (op == "<<") result.Value.v_int32 = left.Value.v_int32 << rightConv.Value.v_int32;
        else if (op == ">>") result.Value.v_int32 = left.Value.v_int32 >> rightConv.Value.v_int32;
        break;

    case TYPE_LONG_INT:
        if (op == "<<") result.Value.v_int64 = left.Value.v_int64 << rightConv.Value.v_int32;
        else if (op == ">>") result.Value.v_int64 = left.Value.v_int64 >> rightConv.Value.v_int32;
        break;

    default:
        semError("неподдерживаемый тип для операции сдвига", "", line, col);
    }

    return result;
}

// Операции сравнения
SemNode Tree::executeComparisonOp(const SemNode& left, const SemNode& right, const string& op, int line, int col) {
    if (!left.hasValue || !right.hasValue) {
        semError("операция с неинициализированными значениями", "", line, col);
    }

    DATA_TYPE resultType = getMaxType(left.DataType, right.DataType);
    SemNode leftConv = castToType(left, resultType, line, col);
    SemNode rightConv = castToType(right, resultType, line, col);

    SemNode result;
    result.DataType = TYPE_BOOL;
    result.hasValue = true;

    if (!interpretationEnabled) return result;

    switch (resultType) {
    case TYPE_SHORT_INT:
        if (op == "<") result.Value.v_bool = leftConv.Value.v_int16 < rightConv.Value.v_int16;
        else if (op == "<=") result.Value.v_bool = leftConv.Value.v_int16 <= rightConv.Value.v_int16;
        else if (op == ">") result.Value.v_bool = leftConv.Value.v_int16 > rightConv.Value.v_int16;
        else if (op == ">=") result.Value.v_bool = leftConv.Value.v_int16 >= rightConv.Value.v_int16;
        else if (op == "==") result.Value.v_bool = leftConv.Value.v_int16 == rightConv.Value.v_int16;
        else if (op == "!=") result.Value.v_bool = leftConv.Value.v_int16 != rightConv.Value.v_int16;
        break;

    case TYPE_INT:
        if (op == "<") result.Value.v_bool = leftConv.Value.v_int32 < rightConv.Value.v_int32;
        else if (op == "<=") result.Value.v_bool = leftConv.Value.v_int32 <= rightConv.Value.v_int32;
        else if (op == ">") result.Value.v_bool = leftConv.Value.v_int32 > rightConv.Value.v_int32;
        else if (op == ">=") result.Value.v_bool = leftConv.Value.v_int32 >= rightConv.Value.v_int32;
        else if (op == "==") result.Value.v_bool = leftConv.Value.v_int32 == rightConv.Value.v_int32;
        else if (op == "!=") result.Value.v_bool = leftConv.Value.v_int32 != rightConv.Value.v_int32;
        break;

    case TYPE_LONG_INT:
        if (op == "<") result.Value.v_bool = leftConv.Value.v_int64 < rightConv.Value.v_int64;
        else if (op == "<=") result.Value.v_bool = leftConv.Value.v_int64 <= rightConv.Value.v_int64;
        else if (op == ">") result.Value.v_bool = leftConv.Value.v_int64 > rightConv.Value.v_int64;
        else if (op == ">=") result.Value.v_bool = leftConv.Value.v_int64 >= rightConv.Value.v_int64;
        else if (op == "==") result.Value.v_bool = leftConv.Value.v_int64 == rightConv.Value.v_int64;
        else if (op == "!=") result.Value.v_bool = leftConv.Value.v_int64 != rightConv.Value.v_int64;
        break;

    case TYPE_BOOL:
        if (op == "==") result.Value.v_bool = left.Value.v_bool == right.Value.v_bool;
        else if (op == "!=") result.Value.v_bool = left.Value.v_bool != right.Value.v_bool;
        else semError("неподдерживаемая операция сравнения для bool", "", line, col);
        break;

    default:
        semError("неподдерживаемый тип для операции сравнения", "", line, col);
    }

    return result;
}

std::string Tree::makeLabel(const Tree* tree) const {
    if (tree == nullptr) return string("<null-tree>");
    SemNode* n = tree->n;
    if (!n) return string("<null-node>");

    ostringstream oss;

    if (!n->id.empty()) oss << n->id;
    else oss << "{}";

    switch (n->DataType) {
    case TYPE_INT:       oss << " (int)"; break;
    case TYPE_SHORT_INT: oss << " (short)"; break;
    case TYPE_LONG_INT:  oss << " (long)"; break;
    case TYPE_BOOL:      oss << " (bool)"; break;
    case TYPE_FUNCT:     oss << " (функция)"; break;
    case TYPE_SCOPE:     oss << " (область)"; break;
    default:             oss << " (?)"; break;
    }

    auto childName = [](const Tree* t)->string {
        if (!t || !t->n) return string("-");
        if (t->n->DataType == TYPE_SCOPE) return string("{}");
        if (t->n->id.empty()) return string("?");
        return t->n->id;
        };

    string rname = childName(tree->Right);
    string lname = childName(tree->Left);

    oss << " (L = " << rname << ", R = " << lname << ")";

    return oss.str();
}

void Tree::print(int indent) {
    if (this == nullptr) return;

    string label = makeLabel(this);
    for (int i = 0; i < indent; ++i) std::cout << ' ';
    std::cout << label << '\n';

    Tree* child = this->Left;
    while (child) {
        child->print(indent + 4);
        child = child->Right;
    }
}

void Tree::print() {
    print(0);
}

void Tree::enableInterpretation() { interpretationEnabled = true; }
void Tree::disableInterpretation() { interpretationEnabled = false; }
bool Tree::isInterpretationEnabled() { return interpretationEnabled; }

// Метод для вывода отладочной информации
void Tree::printDebugInfo(const string& message, int line, int col) {
    if (!debug || !interpretationEnabled) return;

    // Определяем контекст
    string context = "глобальная область";

    // Используем currentFunction для определения контекста
    if (currentFunction && currentFunction->n && !currentFunction->n->id.empty()) {
        context = currentFunction->n->id;
    }
    else {
        // Резервный метод: ищем функцию в дереве
        Tree* cur = Cur;
        while (cur != nullptr) {
            if (cur->n && cur->n->DataType == TYPE_FUNCT && !cur->n->id.empty()) {
                context = cur->n->id;
                break;
            }
            cur = cur->Up;
        }
    }

    // Выводим сообщение с контекстом и позицией
    std::cout << "DEBUG: [" << context << "]";
    if (line > 0) {
        std::cout << " (строка " << line << ":" << col << ")";
    }
    std::cout << " " << message << std::endl;
}

// Метод для вывода присваивания
void Tree::printAssignment(const string& varName, const SemNode& value, int line, int col) {
    if (!debug || !interpretationEnabled) return;

    std::ostringstream oss;
    oss << "Присваивание: " << varName << " = ";

    if (value.hasValue) {
        switch (value.DataType) {
        case TYPE_SHORT_INT:
            oss << value.Value.v_int16 << " (short)";
            break;
        case TYPE_INT:
            oss << value.Value.v_int32 << " (int)";
            break;
        case TYPE_LONG_INT:
            oss << value.Value.v_int64 << " (long)";
            break;
        case TYPE_BOOL:
            oss << (value.Value.v_bool ? "true" : "false") << " (bool)";
            break;
        default:
            oss << "unknown";
        }
    }
    else {
        oss << "неинициализирована";
    }

    printDebugInfo(oss.str(), line, col);
}

// Метод для вывода вызова функции
void Tree::printFunctionCall(const string& funcName, const std::vector<SemNode>& args, int line, int col) {
    if (!debug || !interpretationEnabled) return;

    std::ostringstream oss;
    oss << "Вызов функции: " << funcName << "(";

    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) oss << ", ";
        if (args[i].hasValue) {
            switch (args[i].DataType) {
            case TYPE_SHORT_INT: oss << args[i].Value.v_int16 << " (short)"; break;
            case TYPE_INT: oss << args[i].Value.v_int32 << " (int)"; break;
            case TYPE_LONG_INT: oss << args[i].Value.v_int64 << " (long)"; break;
            case TYPE_BOOL: oss << (args[i].Value.v_bool ? "true" : "false") << " (bool)"; break;
            default: oss << "unknown";
            }
        }
        else {
            oss << "неинициализирована";
        }
    }
    oss << ")";

    printDebugInfo(oss.str(), line, col);

    // Добавляем сообщение о начале выполнения тела функции
    std::ostringstream oss2;
    oss2 << "|--> Начало выполнения тела функции " << funcName;
    printDebugInfo(oss2.str(), line, col);
}

// Метод для вывода арифметической операции
void Tree::printArithmeticOp(const string& op, const SemNode& left, const SemNode& right, const SemNode& result, int line, int col) {
    if (!debug || !interpretationEnabled) return;

    std::ostringstream oss;
    oss << "Арифметическая операция: ";

    // Левый операнд
    if (left.hasValue) {
        switch (left.DataType) {
        case TYPE_SHORT_INT: oss << left.Value.v_int16 << " (short)"; break;
        case TYPE_INT: oss << left.Value.v_int32 << " (int)"; break;
        case TYPE_LONG_INT: oss << left.Value.v_int64 << " (long)"; break;
        case TYPE_BOOL: oss << (left.Value.v_bool ? "true" : "false") << " (bool)"; break;
        default: oss << "unknown";
        }
    }
    else {
        oss << "неинициализирована";
    }

    oss << " " << op << " ";

    // Правый операнд
    if (right.hasValue) {
        switch (right.DataType) {
        case TYPE_SHORT_INT: oss << right.Value.v_int16 << " (short)"; break;
        case TYPE_INT: oss << right.Value.v_int32 << " (int)"; break;
        case TYPE_LONG_INT: oss << right.Value.v_int64 << " (long)"; break;
        case TYPE_BOOL: oss << (right.Value.v_bool ? "true" : "false") << " (bool)"; break;
        default: oss << "unknown";
        }
    }
    else {
        oss << "неинициализирована";
    }

    oss << " = ";

    // Результат
    if (result.hasValue) {
        switch (result.DataType) {
        case TYPE_SHORT_INT: oss << result.Value.v_int16 << " (short)"; break;
        case TYPE_INT: oss << result.Value.v_int32 << " (int)"; break;
        case TYPE_LONG_INT: oss << result.Value.v_int64 << " (long)"; break;
        case TYPE_BOOL: oss << (result.Value.v_bool ? "true" : "false") << " (bool)"; break;
        default: oss << "unknown";
        }
    }
    else {
        oss << "неинициализирована";
    }

    printDebugInfo(oss.str(), line, col);
}

void Tree::enableDebug() { debug = true; }
void Tree::disableDebug() { debug = false; }
bool Tree::isDebugEnabled() { return debug; }

Tree* Tree::cloneSubtree(Tree* node) {
    if (!node) return nullptr;
    return cloneRecursive(node);
}

Tree* Tree::cloneRecursive(const Tree* node) {
    if (!node) return nullptr;
    Tree* copy = new Tree(nullptr, nullptr);
    if (node->n) {
        copy->n = new SemNode(*node->n); // использует copy-ctor SemNode
    }
    copy->Left = cloneRecursive(node->Left);
    copy->Right = cloneRecursive(node->Right);
    return copy;
}

void Tree::fixUpPointers(Tree* copy, Tree* parentUp) {
    if (!copy) return;
    copy->Up = parentUp;
    if (copy->Left) {
        // левый потомок — его Up должен быть текущий 'copy'
        // рекурсивно идём внутрь
        fixUpPointers(copy->Left, copy);
    }
    if (copy->Right) {
        // правый сосед — его Up должен быть тот же parentUp (как в оригинале)
        fixUpPointers(copy->Right, parentUp);
    }
}