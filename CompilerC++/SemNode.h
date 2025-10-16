#pragma once
#include <string>
#include <vector>
#include "DataType.h"

using namespace std;

struct SemNode {
	string id; // имя идентификатора 
	DATA_TYPE DataType; // тип объекта 
	char* Data; // ссылка на значение 
	int Param; // число формальных параметров (для функций) 
	std::vector<DATA_TYPE> ParamTypes; // список типов формальных параметров (для функций)
	int line; // строка объявления (для сообщений об ошибках) 
	int col; // позиция в строке (для сообщений об ошибках) 
};
