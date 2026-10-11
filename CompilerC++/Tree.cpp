#include "Tree.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <functional>
#include <stdexcept>

// Initial values of the static state
Tree* Tree::Root = nullptr;
Tree* Tree::Cur = nullptr;
bool Tree::interpretationEnabled = true; // enabled by default
bool Tree::debug = true;                 // verbose output enabled by default
Tree* Tree::currentFunction = nullptr;
int Tree::recursionDepth = 0;

static const char* dataTypeName(DATA_TYPE t) {
    switch (t) {
    case TYPE_INT:       return "int";
    case TYPE_SHORT_INT: return "short";
    case TYPE_LONG_INT:  return "long";
    case TYPE_BOOL:      return "bool";
    case TYPE_FUNCT:     return "function";
    case TYPE_SCOPE:     return "scope";
    default:             return "unknown";
    }
}

void Tree::enterFunctionCall(const string& funcName, int line, int col) {
    // Check before incrementing so the depth is left unchanged when we abort.
    if (recursionDepth >= MAX_RECURSION_DEPTH) {
        interpError("maximum recursion depth exceeded", funcName, line, col);
    }
    recursionDepth++;
}

void Tree::exitFunctionCall() {
    if (recursionDepth > 0) recursionDepth--;
}

void Tree::printTypeConversionWarning(DATA_TYPE from, DATA_TYPE to, const string& context,
    const string& expression, int line, int col) {

    if (!debug || !interpretationEnabled) return;

    std::cerr << "Warning: implicit conversion from " << dataTypeName(from)
        << " to " << dataTypeName(to) << " in " << context;
    if (!expression.empty()) {
        std::cerr << " of expression " << expression;
    }
    std::cerr << std::endl << "(line " << line << ":" << col << ")" << std::endl;
}

// Print a semantic error and abort the current compilation stage.
void Tree::semError(const string& msg, const string& id, int line, int col) {
    std::cerr << "Semantic error: " << msg;
    if (!id.empty()) std::cerr << " (near '" << id << "')";
    std::cerr << std::endl << "(line " << line << ":" << col << ")" << std::endl;
    throw std::runtime_error("Semantic error");
}

// Print an interpretation error and abort the current execution.
void Tree::interpError(const string& msg, const string& id, int line, int col) {
    std::cerr << "Interpretation error: " << msg;
    if (!id.empty()) std::cerr << " (near '" << id << "')";
    std::cerr << std::endl << "(line " << line << ":" << col << ")" << std::endl;
    throw std::runtime_error("Interpretation error");
}

Tree::Tree(SemNode* node, Tree* up) : n(node), Up(up), Left(nullptr), Right(nullptr) {
    if (Root == nullptr) {
        Root = this;
        Cur = this;
    }
}

// The destructor recursively frees the subtree and the whole sibling chain.
Tree::~Tree() {
    if (n) { delete n; n = nullptr; }
    if (Left) { delete Left; Left = nullptr; }
    if (Right) { delete Right; Right = nullptr; }
}

// Append a new child to this node's child list.
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

// Look for `id` among the direct children of `From`.
Tree* Tree::findUpOneLevel(Tree* From, const string& id) {
    if (From == nullptr) return nullptr;
    Tree* p = From->Left;
    while (p != nullptr) {
        if (p->n && p->n->id == id) return p;
        p = p->Right;
    }
    return nullptr;
}

// Search for `id` walking up through enclosing scopes.
Tree* Tree::findUp(Tree* From, const string& id) {
    Tree* cur = From;
    while (cur != nullptr) {
        Tree* found = findUpOneLevel(cur, id);
        if (found) return found;
        cur = cur->Up;
    }
    return nullptr;
}

// Check whether `a` is already declared in the scope `Addr`.
bool Tree::dupControl(Tree* Addr, const string& a) {
    return findUpOneLevel(Addr, a) != nullptr;
}

// Insert an identifier into the current scope.
Tree* Tree::semInclude(const string& a, DATA_TYPE t, int line, int col) {
    if (Cur == nullptr) {
        semError("internal error: current scope is not set in semInclude", a, line, col);
    }

    if (dupControl(Cur, a)) {
        semError("redeclaration of identifier", a, line, col);
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

        // Functions own a scope child that holds their parameters and locals.
        SemNode* emptyNode = new SemNode();
        emptyNode->id = "";
        emptyNode->DataType = TYPE_SCOPE;
        emptyNode->Param = 0;
        emptyNode->line = line;
        emptyNode->col = col;

        funcNode->setLeft(emptyNode);

        return funcNode;
    }
    else {
        Cur->setLeft(node);
        Tree* added = Cur->Left;
        while (added->Right) added = added->Right;

        return added;
    }
}

// Insert a named constant with a known value.
Tree* Tree::semIncludeConstant(const string& a, DATA_TYPE t, const string& value, int line, int col) {
    Tree* node = semInclude(a, t, line, col);
    if (node && node->n) {
        node->n->hasValue = true;
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
            semError("invalid constant format: " + string(e.what()), a, line, col);
        }
    }
    return node;
}

void Tree::semSetParam(Tree* Addr, int n) {
    if (Addr == nullptr || Addr->n == nullptr) {
        semError("semSetParam: invalid function address");
    }
    Addr->n->Param = n;
}

void Tree::semSetParamTypes(Tree* Addr, const std::vector<DATA_TYPE>& types) {
    if (Addr == nullptr || Addr->n == nullptr) {
        semError("semSetParamTypes: invalid function address");
    }
    Addr->n->ParamTypes = types;
    Addr->n->Param = static_cast<int>(types.size());
}

void Tree::semControlParamTypes(Tree* Addr, const std::vector<DATA_TYPE>& argTypes, int line, int col) {
    if (Addr == nullptr || Addr->n == nullptr) {
        semError("semControlParamTypes: invalid function address");
    }
    const std::vector<DATA_TYPE>& formal = Addr->n->ParamTypes;
    if (formal.size() != argTypes.size()) {
        semError("wrong number of arguments in call to function", Addr->n->id, line, col);
    }
    for (size_t i = 0; i < formal.size(); ++i) {
        bool formalIsInt = (formal[i] == TYPE_INT || formal[i] == TYPE_SHORT_INT || formal[i] == TYPE_LONG_INT);
        bool argIsInt = (argTypes[i] == TYPE_INT || argTypes[i] == TYPE_SHORT_INT || argTypes[i] == TYPE_LONG_INT);
        if (formalIsInt && argIsInt) continue;
        if (formal[i] == argTypes[i]) continue;
        semError("argument type mismatch in call to function", Addr->n->id, line, col);
    }
}

Tree* Tree::semGetVar(const string& a, int line, int col) {
    Tree* v = findUp(Cur, a);
    if (v == nullptr) {
        semError("undeclared identifier", a, line, col);
    }
    if (v->n->DataType == TYPE_FUNCT) {
        semError("identifier is a function and cannot be used as a variable", a, line, col);
    }
    return v;
}

Tree* Tree::semGetFunct(const string& a, int line, int col) {
    Tree* v = findUp(Cur, a);
    if (v == nullptr) {
        semError("undeclared function", a, line, col);
    }
    if (v->n->DataType != TYPE_FUNCT) {
        semError("identifier is not a function", a, line, col);
    }
    return v;
}

Tree* Tree::semEnterBlock(int line, int col) {
    if (Cur == nullptr) {
        semError("semEnterBlock: current scope is not set");
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
        semError("semExitBlock: current scope is not set");
    }
    if (Cur->Up == nullptr) {
        semError("semExitBlock: attempt to leave the root scope");
    }
    Cur = Cur->Up;
}

void Tree::setVarValue(const string& name, const SemNode& value, int line, int col) {
    Tree* varNode = Cur->semGetVar(name, line, col);

    if (!value.hasValue) {
        interpError("attempt to assign an empty value to a variable", name, line, col);
    }

    if (!canImplicitCast(value.DataType, varNode->n->DataType)) {
        semError("incompatible types in assignment", name, line, col);
    }

    // Range check for the target integer type.
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
        if (debug) {
            std::cerr << "Warning: value " << originalValue
                << " is truncated when converting to "
                << dataTypeName(varNode->n->DataType);
            std::cerr << std::endl << "(line " << line << ":" << col << ")" << std::endl;
        }
    }
    else if (value.DataType != varNode->n->DataType && debug) {
        printTypeConversionWarning(value.DataType, varNode->n->DataType,
            "assignment", name + " = ...", line, col);
    }

    SemNode converted = castToType(value, varNode->n->DataType, line, col);

    // In semantic-only mode we only mark the variable as initialized.
    if (!Tree::isInterpretationEnabled()) {
        varNode->n->hasValue = true;
        return;
    }

    varNode->n->Value = converted.Value;
    varNode->n->hasValue = true;

    printAssignment(name, converted, line, col);
}

SemNode Tree::getVarValue(const string& name, int line, int col) {
    Tree* varNode = Cur->semGetVar(name, line, col);
    if (!varNode->n->hasValue) {
        semError("use of uninitialized variable", name, line, col);
    }
    return *(varNode->n);
}

// Type promotion rules for binary integer operations.
DATA_TYPE Tree::getMaxType(DATA_TYPE t1, DATA_TYPE t2) {
    if (t1 == TYPE_LONG_INT || t2 == TYPE_LONG_INT) return TYPE_LONG_INT;
    if (t1 == TYPE_INT || t2 == TYPE_INT) return TYPE_INT;
    return TYPE_SHORT_INT;
}

bool Tree::canImplicitCast(DATA_TYPE from, DATA_TYPE to) {
    bool fromIsInt = (from == TYPE_SHORT_INT || from == TYPE_INT || from == TYPE_LONG_INT);
    bool toIsInt = (to == TYPE_SHORT_INT || to == TYPE_INT || to == TYPE_LONG_INT);
    return (fromIsInt && toIsInt) || (from == TYPE_BOOL && to == TYPE_BOOL);
}

// Convert a value to the requested type.
SemNode Tree::castToType(const SemNode& value, DATA_TYPE targetType, int line, int col, bool showWarning) {
    (void)showWarning;
    if (value.DataType == targetType) {
        return value;
    }

    SemNode result;
    result.DataType = targetType;
    result.hasValue = value.hasValue;

    if (!value.hasValue) return result;

    long long originalValue = 0;
    switch (value.DataType) {
    case TYPE_SHORT_INT: originalValue = value.Value.v_int16; break;
    case TYPE_INT:       originalValue = value.Value.v_int32; break;
    case TYPE_LONG_INT:  originalValue = value.Value.v_int64; break;
    default: break;
    }

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
            semError("invalid conversion of an integer value to bool", "", line, col);
        }
        break;
    default:
        semError("unknown target type in conversion", "", line, col);
    }

    return result;
}

// Arithmetic operations
SemNode Tree::executeArithmeticOp(const SemNode& left, const SemNode& right, const string& op, int line, int col) {
    if (!left.hasValue || !right.hasValue) {
        semError("operation on uninitialized values", "", line, col);
    }

    if (left.DataType != right.DataType && debug) {
        printTypeConversionWarning(left.DataType, right.DataType,
            "arithmetic operation", "", line, col);
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
            if (rightConv.Value.v_int16 == 0) interpError("division by zero", "", line, col);
            result.Value.v_int16 = leftConv.Value.v_int16 / rightConv.Value.v_int16;
        }
        else if (op == "%") {
            if (rightConv.Value.v_int16 == 0) interpError("division by zero", "", line, col);
            result.Value.v_int16 = leftConv.Value.v_int16 % rightConv.Value.v_int16;
        }
        break;

    case TYPE_INT:
        if (op == "+") result.Value.v_int32 = leftConv.Value.v_int32 + rightConv.Value.v_int32;
        else if (op == "-") result.Value.v_int32 = leftConv.Value.v_int32 - rightConv.Value.v_int32;
        else if (op == "*") result.Value.v_int32 = leftConv.Value.v_int32 * rightConv.Value.v_int32;
        else if (op == "/") {
            if (rightConv.Value.v_int32 == 0) interpError("division by zero", "", line, col);
            result.Value.v_int32 = leftConv.Value.v_int32 / rightConv.Value.v_int32;
        }
        else if (op == "%") {
            if (rightConv.Value.v_int32 == 0) interpError("division by zero", "", line, col);
            result.Value.v_int32 = leftConv.Value.v_int32 % rightConv.Value.v_int32;
        }
        break;

    case TYPE_LONG_INT:
        if (op == "+") result.Value.v_int64 = leftConv.Value.v_int64 + rightConv.Value.v_int64;
        else if (op == "-") result.Value.v_int64 = leftConv.Value.v_int64 - rightConv.Value.v_int64;
        else if (op == "*") result.Value.v_int64 = leftConv.Value.v_int64 * rightConv.Value.v_int64;
        else if (op == "/") {
            if (rightConv.Value.v_int64 == 0) interpError("division by zero", "", line, col);
            result.Value.v_int64 = leftConv.Value.v_int64 / rightConv.Value.v_int64;
        }
        else if (op == "%") {
            if (rightConv.Value.v_int64 == 0) interpError("division by zero", "", line, col);
            result.Value.v_int64 = leftConv.Value.v_int64 % rightConv.Value.v_int64;
        }
        break;

    default:
        semError("unsupported type in arithmetic operation", "", line, col);
    }

    if (debug && interpretationEnabled) {
        printArithmeticOp(op, leftConv, rightConv, result, line, col);
    }

    return result;
}

// Shift operations. The result keeps the type of the left operand.
SemNode Tree::executeShiftOp(const SemNode& left, const SemNode& right, const string& op, int line, int col) {
    if (!left.hasValue || !right.hasValue) {
        semError("operation on uninitialized values", "", line, col);
    }

    SemNode result;
    result.DataType = left.DataType;
    result.hasValue = true;

    if (!interpretationEnabled) return result;

    SemNode rightConv = castToType(right, TYPE_INT, line, col);
    int shift = rightConv.Value.v_int32;
    if (shift < 0) {
        interpError("negative shift count", "", line, col);
    }

    switch (left.DataType) {
    case TYPE_SHORT_INT:
        if (op == "<<") result.Value.v_int16 = static_cast<int16_t>(left.Value.v_int16 << (shift & 15));
        else if (op == ">>") result.Value.v_int16 = static_cast<int16_t>(left.Value.v_int16 >> (shift & 15));
        break;

    case TYPE_INT:
        if (op == "<<") result.Value.v_int32 = left.Value.v_int32 << (shift & 31);
        else if (op == ">>") result.Value.v_int32 = left.Value.v_int32 >> (shift & 31);
        break;

    case TYPE_LONG_INT:
        if (op == "<<") result.Value.v_int64 = left.Value.v_int64 << (shift & 63);
        else if (op == ">>") result.Value.v_int64 = left.Value.v_int64 >> (shift & 63);
        break;

    default:
        semError("unsupported type in shift operation", "", line, col);
    }

    return result;
}

// Comparison operations. The result is always bool.
SemNode Tree::executeComparisonOp(const SemNode& left, const SemNode& right, const string& op, int line, int col) {
    if (!left.hasValue || !right.hasValue) {
        semError("operation on uninitialized values", "", line, col);
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
        else semError("unsupported comparison for bool", "", line, col);
        break;

    default:
        semError("unsupported type in comparison operation", "", line, col);
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

    oss << " (" << dataTypeName(n->DataType) << ")";

    auto childName = [](const Tree* t)->string {
        if (!t || !t->n) return string("-");
        if (t->n->DataType == TYPE_SCOPE) return string("{}");
        if (t->n->id.empty()) return string("?");
        return t->n->id;
        };

    string rname = childName(tree->Right);
    string lname = childName(tree->Left);

    oss << " (L = " << lname << ", R = " << rname << ")";

    return oss.str();
}

void Tree::print(int indent) {
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

void Tree::enableDebug() { debug = true; }
void Tree::disableDebug() { debug = false; }
bool Tree::isDebugEnabled() { return debug; }

// Print a debug message prefixed with the current function context and position.
void Tree::printDebugInfo(const string& message, int line, int col) {
    if (!debug || !interpretationEnabled) return;

    string context = "global scope";

    if (currentFunction && currentFunction->n && !currentFunction->n->id.empty()) {
        context = currentFunction->n->id;
    }
    else {
        Tree* cur = Cur;
        while (cur != nullptr) {
            if (cur->n && cur->n->DataType == TYPE_FUNCT && !cur->n->id.empty()) {
                context = cur->n->id;
                break;
            }
            cur = cur->Up;
        }
    }

    std::cout << "DEBUG: [" << context << "]";
    if (line > 0) {
        std::cout << " (line " << line << ":" << col << ")";
    }
    std::cout << " " << message << std::endl;
}

void Tree::printAssignment(const string& varName, const SemNode& value, int line, int col) {
    if (!debug || !interpretationEnabled) return;

    std::ostringstream oss;
    oss << "Assignment: " << varName << " = ";

    if (value.hasValue) {
        switch (value.DataType) {
        case TYPE_SHORT_INT: oss << value.Value.v_int16 << " (short)"; break;
        case TYPE_INT:       oss << value.Value.v_int32 << " (int)"; break;
        case TYPE_LONG_INT:  oss << value.Value.v_int64 << " (long)"; break;
        case TYPE_BOOL:      oss << (value.Value.v_bool ? "true" : "false") << " (bool)"; break;
        default:             oss << "unknown";
        }
    }
    else {
        oss << "uninitialized";
    }

    printDebugInfo(oss.str(), line, col);
}

void Tree::printFunctionCall(const string& funcName, const std::vector<SemNode>& args, int line, int col) {
    if (!debug || !interpretationEnabled) return;

    std::ostringstream oss;
    oss << "Function call: " << funcName << "(";

    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) oss << ", ";
        if (args[i].hasValue) {
            switch (args[i].DataType) {
            case TYPE_SHORT_INT: oss << args[i].Value.v_int16 << " (short)"; break;
            case TYPE_INT:       oss << args[i].Value.v_int32 << " (int)"; break;
            case TYPE_LONG_INT:  oss << args[i].Value.v_int64 << " (long)"; break;
            case TYPE_BOOL:      oss << (args[i].Value.v_bool ? "true" : "false") << " (bool)"; break;
            default:             oss << "unknown";
            }
        }
        else {
            oss << "uninitialized";
        }
    }
    oss << ")";

    printDebugInfo(oss.str(), line, col);

    std::ostringstream oss2;
    oss2 << "|--> Entering function body " << funcName;
    printDebugInfo(oss2.str(), line, col);
}

void Tree::printArithmeticOp(const string& op, const SemNode& left, const SemNode& right, const SemNode& result, int line, int col) {
    if (!debug || !interpretationEnabled) return;

    auto printVal = [](std::ostringstream& os, const SemNode& v) {
        if (!v.hasValue) { os << "uninitialized"; return; }
        switch (v.DataType) {
        case TYPE_SHORT_INT: os << v.Value.v_int16 << " (short)"; break;
        case TYPE_INT:       os << v.Value.v_int32 << " (int)"; break;
        case TYPE_LONG_INT:  os << v.Value.v_int64 << " (long)"; break;
        case TYPE_BOOL:      os << (v.Value.v_bool ? "true" : "false") << " (bool)"; break;
        default:             os << "unknown";
        }
        };

    std::ostringstream oss;
    oss << "Arithmetic operation: ";
    printVal(oss, left);
    oss << " " << op << " ";
    printVal(oss, right);
    oss << " = ";
    printVal(oss, result);

    printDebugInfo(oss.str(), line, col);
}

void Tree::reset() {
    if (Root) delete Root;
    Root = nullptr;
    Cur = nullptr;
    interpretationEnabled = true;
    debug = false;
    currentFunction = nullptr;
    recursionDepth = 0;
}
