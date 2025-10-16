#pragma once
#include <string>
#include <vector>
#include "DataType.h"

using namespace std;

struct SemNode {
	string id; // им€ идентификатора 
	DATA_TYPE DataType; // тип объекта 
	char* Data; // ссылка на значение 
	int Param; // число формальных параметров (дл€ функций) 
	std::vector<DATA_TYPE> ParamTypes; // список типов формальных параметров (дл€ функций)
	int line; // строка объ€влени€ (дл€ сообщений об ошибках) 
	int col; // позици€ в строке (дл€ сообщений об ошибках) 
};
