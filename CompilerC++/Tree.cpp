#include "Tree.h"

#include <iostream>
#include <sstream>

// Инициализация статических полей
Tree* Tree::Root = nullptr;
Tree* Tree::Cur = nullptr;

// печать семантической ошибки
void Tree::semError(const string& msg, const string& id, int line, int col) {
    std::cerr << "Семантическая ошибка: " << msg;
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
    // рекурсивно очищаем детей/соседей
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
        // найти последний правый сосед среди потомков
        Tree* p = this->Left;
        while (p->Right) p = p->Right;
        p->Right = newNode;
        newNode->Up = this;
    }
}

// Вставка правого соседа
void Tree::setRight(SemNode* Data) {
    Tree* newNode = new Tree(Data, this->Up);
    // вставляем после текущего узла
    newNode->Right = this->Right;
    this->Right = newNode;
    newNode->Up = this->Up; // тот же родитель, что и у текущего узла
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
    if (findUpOneLevel(Addr, a) == nullptr) return false;
    return true;
}

// добавляет идентификатор в текущую область Cur
Tree* Tree::semInclude(const string& a, DATA_TYPE t, int line, int col) {
    if (Cur == nullptr) {
        semError("внутренняя ошибка: текущая область не установлена при SemInclude", a, line, col);
    }

    // Проверка повтора в текущем уровне
    if (dupControl(Cur, a)) {
        semError("повторное описание идентификатора", a, line, col);
    }

    // создаём SemNode
    SemNode* node = new SemNode();
    node->id = a;
    node->DataType = t;
    node->Data = nullptr;
    node->Param = 0;
    node->line = line;
    node->col = col;

    // Если это функция — создаём узел функции и правый пустой узел для тела
    if (t == TYPE_FUNCT) {
        // вставляем функцию как новый дочерний элемент текущей области
        Cur->setLeft(node); // ЯВНО через Cur

        // указатель на созданную функцию — последний добавленный потомок
        Tree* funcNode = Cur->Left;
        while (funcNode->Right) funcNode = funcNode->Right;

        // создаём пустой правый узел для тела функции
        SemNode* emptyNode = new SemNode();
        emptyNode->id = "";
        emptyNode->DataType = TYPE_SCOPE;
        emptyNode->Param = 0;
        emptyNode->line = line;
        emptyNode->col = col;

        // вставляем emptyNode как правого потомка у funcNode
        funcNode->setRight(emptyNode); // Использовать метод SetRight
        return funcNode;
    }
    else {
        // Обычная переменная/параметр/объявление
        Cur->setLeft(node); // явно через Cur
        // возвращаем указатель на только что добавленный узел
        Tree* added = Cur->Left;
        while (added->Right) added = added->Right;
        return added;
    }
}

// записать число формальных параметров для функции Addr
void Tree::semSetParam(Tree* Addr, int n) {
    if (Addr == nullptr || Addr->n == nullptr) {
        semError("SemSetParam: неверный адрес функции");
    }
    Addr->n->Param = n;
}

// сохранить типы формальных параметров
void Tree::semSetParamTypes(Tree* Addr, const std::vector<DATA_TYPE>& types) {
    if (Addr == nullptr || Addr->n == nullptr) {
        semError("SemSetParamTypes: неверный адрес функции");
    }
    Addr->n->ParamTypes = types;
    Addr->n->Param = (int)types.size();
}

// проверка числа и типов аргументов
void Tree::semControlParamTypes(Tree* Addr, const std::vector<DATA_TYPE>& argTypes, int line, int col) {
    if (Addr == nullptr || Addr->n == nullptr) {
        semError("SemControlParamTypes: неверный адрес функции");
    }
    const std::vector<DATA_TYPE>& formal = Addr->n->ParamTypes;
    if (formal.size() != argTypes.size()) {
        semError("неверное число параметров у функции", Addr->n->id, line, col);
    }
    for (size_t i = 0; i < formal.size(); ++i) {
        // считаем short/long/int совместимыми между собой
        bool formalIsInt = (formal[i] == TYPE_INT || formal[i] == TYPE_SHORT_INT || formal[i] == TYPE_LONG_INT);
        bool argIsInt = (argTypes[i] == TYPE_INT || argTypes[i] == TYPE_SHORT_INT || argTypes[i] == TYPE_LONG_INT);
        if (formalIsInt && argIsInt) continue;
        if (formal[i] == argTypes[i]) continue;
        // иначе ошибка
        semError("несоответствие типов параметров у функции", Addr->n->id, line, col);
    }
}

// найти переменную (не функцию) по имени (в видимых областях)
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

// найти функцию по имени
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

// создать узел-область и перейти в него
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

    // вставляем новую область как дочерний элемент текущего Cur
    // (т.е. она будет видимой как локальная область для последующих SemInclude)
    setLeft(sn);

    // возвращаем указатель на созданный узел (последний дочерний)
    Tree* created = Cur->Left;
    while (created->Right) created = created->Right;

    // переключаем текущую область на созданную
    Cur = created;
    return created;
}

// вернуться на уровень вверх
void Tree::semExitBlock() {
    if (Cur == nullptr) {
        semError("SemExitBlock: текущая область не установлена");
    }
    if (Cur->Up == nullptr) {
        semError("SemExitBlock: попытка выйти из корневой области");
    }
    Cur = Cur->Up;
}

// создать строковое представление узла для печати
std::string Tree::makeLabel(const Tree* tree) const {
    if (tree == nullptr) return std::string("<null-tree>");
    SemNode* n = tree->n;
    if (!n) return std::string("<null-node>");

    std::ostringstream oss;

    if (!n->id.empty()) oss << n->id;
    else oss << "{}";

    // тип
    switch (n->DataType) {
    case TYPE_INT:       oss << " (int)"; break;
    case TYPE_SHORT_INT: oss << " (short)"; break;
    case TYPE_LONG_INT:  oss << " (long)"; break;
    case TYPE_BOOL:      oss << " (bool)"; break;
    case TYPE_FUNCT:     oss << " (функция)"; break;
    case TYPE_SCOPE:     oss << " (область)"; break;
    default:             oss << " (?)"; break;
    }

    // берем имена правого и левого потомков
    auto childName = [](const Tree* t)->std::string {
        if (!t || !t->n) return std::string("-");
        if (t->n->DataType == TYPE_SCOPE) return std::string("{}");
        if (t->n->id.empty()) return std::string("?");
        return t->n->id;
    };

    std::string rname = childName(tree->Right);
    std::string lname = childName(tree->Left);

    oss << " (R = " << rname << ", L = " << lname << ")";

    return oss.str();
}

void Tree::print(int indent) {
    if (this == nullptr) return;

    // печатаем текущий узел
    std::string label = makeLabel(this);
    for (int i = 0; i < indent; ++i) std::cout << ' ';
    std::cout << label << '\n';

	// затем рекурсивно печатаем всех потомков
    Tree* child = this->Left;
    while (child) {
        child->print(indent + 4); // сдвиг для уровня (4 пробела)
        child = child->Right; // переходим к следующему
    }
}

// начать с нулевого отступа
void Tree::print() {
    print(0);
}