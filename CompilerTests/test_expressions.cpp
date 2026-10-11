#include "test_helpers.h"

static SemNode intNode(int32_t v) {
    SemNode n;
    n.DataType = TYPE_INT;
    n.hasValue = true;
    n.Value.v_int32 = v;
    return n;
}

static SemNode shortNode(int16_t v) {
    SemNode n;
    n.DataType = TYPE_SHORT_INT;
    n.hasValue = true;
    n.Value.v_int16 = v;
    return n;
}

TEST(Expression_Arithmetic) {
    Tree::disableDebug();
    CHECK_EQ(Tree::executeArithmeticOp(intNode(5), intNode(3), "+", 1, 1).Value.v_int32, 8);
    CHECK_EQ(Tree::executeArithmeticOp(intNode(5), intNode(3), "-", 1, 1).Value.v_int32, 2);
    CHECK_EQ(Tree::executeArithmeticOp(intNode(5), intNode(3), "*", 1, 1).Value.v_int32, 15);
    CHECK_EQ(Tree::executeArithmeticOp(intNode(20), intNode(4), "/", 1, 1).Value.v_int32, 5);
    CHECK_EQ(Tree::executeArithmeticOp(intNode(20), intNode(3), "%", 1, 1).Value.v_int32, 2);
}

TEST(Expression_DivisionByZeroThrows) {
    Tree::disableDebug();
    CHECK_THROWS(Tree::executeArithmeticOp(intNode(10), intNode(0), "/", 1, 1));
    CHECK_THROWS(Tree::executeArithmeticOp(intNode(10), intNode(0), "%", 1, 1));
}

TEST(Expression_Comparison) {
    Tree::disableDebug();
    CHECK(Tree::executeComparisonOp(intNode(5), intNode(10), "<", 1, 1).Value.v_bool);
    CHECK(!Tree::executeComparisonOp(intNode(10), intNode(5), "<", 1, 1).Value.v_bool);
    CHECK(Tree::executeComparisonOp(intNode(5), intNode(5), "==", 1, 1).Value.v_bool);
    CHECK(!Tree::executeComparisonOp(intNode(5), intNode(5), "!=", 1, 1).Value.v_bool);
    CHECK(Tree::executeComparisonOp(intNode(5), intNode(5), ">=", 1, 1).Value.v_bool);
}

TEST(Expression_ShiftOperations) {
    Tree::disableDebug();
    CHECK_EQ(Tree::executeShiftOp(intNode(4), intNode(2), "<<", 1, 1).Value.v_int32, 16);
    CHECK_EQ(Tree::executeShiftOp(intNode(16), intNode(2), ">>", 1, 1).Value.v_int32, 4);
}

TEST(Expression_MixedTypePromotion) {
    Tree::disableDebug();
    SemNode result = Tree::executeArithmeticOp(shortNode(2), intNode(3), "+", 1, 1);
    CHECK_EQ((int)result.DataType, (int)TYPE_INT);
    CHECK_EQ(result.Value.v_int32, 5);
}

TEST(Expression_MaxType) {
    CHECK_EQ((int)Tree::getMaxType(TYPE_SHORT_INT, TYPE_INT), (int)TYPE_INT);
    CHECK_EQ((int)Tree::getMaxType(TYPE_INT, TYPE_LONG_INT), (int)TYPE_LONG_INT);
    CHECK_EQ((int)Tree::getMaxType(TYPE_SHORT_INT, TYPE_SHORT_INT), (int)TYPE_SHORT_INT);
}

TEST(Expression_ImplicitCastRules) {
    CHECK(Tree::canImplicitCast(TYPE_INT, TYPE_LONG_INT));
    CHECK(Tree::canImplicitCast(TYPE_LONG_INT, TYPE_SHORT_INT));
    CHECK(Tree::canImplicitCast(TYPE_BOOL, TYPE_BOOL));
    CHECK(!Tree::canImplicitCast(TYPE_BOOL, TYPE_INT));
    CHECK(!Tree::canImplicitCast(TYPE_INT, TYPE_BOOL));
}

TEST(Expression_CastTruncation) {
    Tree::disableDebug();
    SemNode wide = intNode(70000);
    SemNode truncated = Tree::castToType(wide, TYPE_SHORT_INT, 1, 1);
    CHECK_EQ((int)truncated.DataType, (int)TYPE_SHORT_INT);
    CHECK_EQ((int)truncated.Value.v_int16, (int)static_cast<int16_t>(70000));
}

TEST(Expression_OperationOnUninitializedThrows) {
    Tree::disableDebug();
    SemNode empty; // hasValue == false
    CHECK_THROWS(Tree::executeArithmeticOp(intNode(1), empty, "+", 1, 1));
}
