#include "test_helpers.h"

TEST(Semantics_IncludeVariableAndFind) {
    resetSymbolTable();
    Tree::Cur->semInclude("x", TYPE_INT, 1, 1);
    Tree* found = Tree::Cur->findUp(Tree::Cur, "x");
    CHECK(found != nullptr);
    CHECK_EQ(found->n->id, string("x"));
    CHECK_EQ((int)found->n->DataType, (int)TYPE_INT);
}

TEST(Semantics_DuplicateDetection) {
    resetSymbolTable();
    Tree::Cur->semInclude("x", TYPE_INT, 1, 1);
    CHECK(Tree::Cur->dupControl(Tree::Cur, "x"));
    CHECK_THROWS(Tree::Cur->semInclude("x", TYPE_INT, 1, 1));
}

TEST(Semantics_FindVariableInParentScope) {
    resetSymbolTable();
    Tree::Cur->semInclude("x", TYPE_INT, 1, 1);
    Tree::Cur->semEnterBlock(2, 1);
    Tree* found = Tree::Cur->findUp(Tree::Cur, "x");
    CHECK(found != nullptr);
    CHECK_EQ(found->n->id, string("x"));
    Tree::Cur->semExitBlock();
}

TEST(Semantics_ShadowingInNestedScope) {
    resetSymbolTable();
    Tree::Cur->semInclude("x", TYPE_INT, 1, 1);
    Tree::Cur->semEnterBlock(2, 1);
    CHECK(!Tree::Cur->dupControl(Tree::Cur, "x"));
    Tree::Cur->semInclude("x", TYPE_BOOL, 2, 2);
    CHECK(Tree::Cur->dupControl(Tree::Cur, "x"));
    Tree::Cur->semExitBlock();
}

TEST(Semantics_UndeclaredIdentifierThrows) {
    resetSymbolTable();
    CHECK_THROWS(Tree::Cur->semGetVar("missing", 1, 1));
}

TEST(Semantics_FunctionUsedAsVariableThrows) {
    resetSymbolTable();
    Tree::Cur->semInclude("f", TYPE_FUNCT, 1, 1);
    CHECK_THROWS(Tree::Cur->semGetVar("f", 1, 1));
}

TEST(Semantics_VariableUsedAsFunctionThrows) {
    resetSymbolTable();
    Tree::Cur->semInclude("v", TYPE_INT, 1, 1);
    CHECK_THROWS(Tree::Cur->semGetFunct("v", 1, 1));
}

TEST(Semantics_ParameterCountAndTypes) {
    resetSymbolTable();
    Tree* fn = Tree::Cur->semInclude("f", TYPE_FUNCT, 1, 1);
    std::vector<DATA_TYPE> types = { TYPE_INT, TYPE_SHORT_INT };
    fn->semSetParamTypes(fn, types);
    CHECK_EQ(fn->n->Param, 2);

    // All integer types are mutually compatible.
    std::vector<DATA_TYPE> okArgs = { TYPE_LONG_INT, TYPE_INT };
    fn->semControlParamTypes(fn, okArgs, 1, 1);

    // Wrong number of arguments.
    std::vector<DATA_TYPE> none;
    CHECK_THROWS(fn->semControlParamTypes(fn, none, 1, 1));

    // Incompatible argument type.
    std::vector<DATA_TYPE> badArgs = { TYPE_BOOL, TYPE_INT };
    CHECK_THROWS(fn->semControlParamTypes(fn, badArgs, 1, 1));
}

TEST(Semantics_ConstantStoresValue) {
    resetSymbolTable();
    Tree* c = Tree::Cur->semIncludeConstant("c", TYPE_INT, "100", 1, 1);
    CHECK(c != nullptr);
    CHECK(c->n->hasValue);
    CHECK_EQ(c->n->Value.v_int32, 100);
}

TEST(Semantics_UninitializedVariableReadThrows) {
    resetSymbolTable();
    Tree::Cur->semInclude("u", TYPE_INT, 1, 1);
    CHECK_THROWS(Tree::getVarValue("u", 1, 1));
}

TEST(Semantics_AssignAndReadValue) {
    resetSymbolTable();
    Tree::Cur->semInclude("y", TYPE_INT, 1, 1);

    SemNode value;
    value.DataType = TYPE_INT;
    value.hasValue = true;
    value.Value.v_int32 = 42;

    Tree::setVarValue("y", value, 1, 1);
    SemNode read = Tree::getVarValue("y", 1, 1);
    CHECK(read.hasValue);
    CHECK_EQ(read.Value.v_int32, 42);
}
