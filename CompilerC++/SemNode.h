#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "DataType.h"

using namespace std;

// A single entry of the symbol table (variable, parameter, function or scope).
struct SemNode {
    string id;          // identifier name (empty for scopes)
    DATA_TYPE DataType; // kind/type of the described object

    bool hasValue = false; // whether a constant/runtime value is known

    union {
        int16_t v_int16;
        int32_t v_int32;
        int64_t v_int64;
        bool    v_bool;
    } Value; // the value itself

    int Param;                          // number of formal parameters (functions)
    std::vector<DATA_TYPE> ParamTypes;  // types of formal parameters (functions)
    int line;                           // declaration line (for diagnostics)
    int col;                            // declaration column (for diagnostics)

    // Positions of the function body inside the scanner buffer. They are used by
    // the interpreter to jump to the beginning of a body and execute it again.
    size_t bodyStartPos = 0;
    size_t bodyEndPos = 0;

    SemNode() : id(""), DataType(TYPE_INT), hasValue(false), Param(0),
        line(0), col(0), bodyStartPos(0), bodyEndPos(0) {
        Value.v_int64 = 0;
    }

    SemNode(const SemNode& other)
        : id(other.id),
        DataType(other.DataType),
        hasValue(other.hasValue),
        Param(other.Param),
        ParamTypes(other.ParamTypes),
        line(other.line),
        col(other.col),
        bodyStartPos(other.bodyStartPos),
        bodyEndPos(other.bodyEndPos)
    {
        Value = other.Value;
    }

    SemNode& operator=(const SemNode& other) {
        if (this == &other) return *this;
        id = other.id;
        DataType = other.DataType;
        hasValue = other.hasValue;
        Param = other.Param;
        ParamTypes = other.ParamTypes;
        Value = other.Value;
        line = other.line;
        col = other.col;
        bodyStartPos = other.bodyStartPos;
        bodyEndPos = other.bodyEndPos;
        return *this;
    }
};
