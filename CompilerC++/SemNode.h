#pragma once
#include <string>
#include <vector>
#include "DataType.h"

using namespace std;

struct SemNode {
	string id; // ��� �������������� 
	DATA_TYPE DataType; // ��� ������� 
	char* Data; // ������ �� �������� 
	int Param; // ����� ���������� ���������� (��� �������) 
	std::vector<DATA_TYPE> ParamTypes; // ������ ����� ���������� ���������� (��� �������)
	int line; // ������ ���������� (��� ��������� �� �������) 
	int col; // ������� � ������ (��� ��������� �� �������) 
};
