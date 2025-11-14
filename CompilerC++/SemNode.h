#pragma once
#include <string>
#include <vector>
#include "DataType.h"

using namespace std;

struct SemNode {
    string id; // имя идентификатора 
    DATA_TYPE DataType; // тип объекта 

    bool hasValue = false; // есть ли известное константное значение

    union {
        int16_t v_int16;
        int32_t v_int32;
        int64_t v_int64;
        bool    v_bool;
    } Value; // само значение

    int Param; // число формальных параметров (для функций) 
    std::vector<DATA_TYPE> ParamTypes; // список типов формальных параметров (для функций)
    int line; // строка объявления (для сообщений об ошибках) 
    int col; // позиция в строке (для сообщений об ошибках) 

    // Позиции тела функции в исходном тексте (индексы/смещение в буфере Scanner)
    // Используются при интерпретации: перейти на начало тела и повторно распарсить/выполнить.
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