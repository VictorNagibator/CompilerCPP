#pragma once
#include "SemNode.h"

class Tree {
public:
    SemNode* n;    // ������ ����
    Tree* Up;      // �������� (������� �������)
    Tree* Left;    // ������ ��������� ������� (����� ������)
    Tree* Right;   // ��������� ������� �� ��� �� ������ (������ �����)

    // ������� ������� (������/������� ����) -- �����������
    static Tree* Root;
    static Tree* Cur;

    // ������������ / ����������
    Tree(SemNode* node = nullptr, Tree* up = nullptr);
    ~Tree();

    // ���������� �������: ������� ������/������� ��������� (������� ����� ����)
    void SetLeft(SemNode* Data);   // �������� ��� ������ �������� ������� �������� ����
    void SetRight(SemNode* Data);  // �������� ��� ������� ������ �������� ���� 

    // �����: ������� ���������
    Tree* FindUp(Tree* From, const string& id);        // ����� � ������� � ������� ��������
    Tree* FindUpOneLevel(Tree* From, const string& id);// ����� ������ � ������� ������ (����� ����� From)

    // ������������� ��������
    // ��������� �������������� a � ������� �������; ���������� ��������� �� ��������� ���� (Tree *)
    // ���� t == TYPE_FUNCT - �������� ���� ������� � � ���� �������� ������ ������ ���� ��� ����,
    // ������������ ��������� �� ���� ������� (�� �� ������ ������ ����)
    Tree* SemInclude(const string& a, DATA_TYPE t, int line, int col);

    // ���������� ����� ���������� ���������� ��� �������
    void SemSetParam(Tree* Addr, int n);

    // ���������� ������ ����� ���������� ���������� ��� �������
    void SemSetParamTypes(Tree* Addr, const std::vector<DATA_TYPE>& types);

    // ��������� ����������� ��������� �� ����� � ����� (�������� ��� Call)
    void SemControlParamTypes(Tree* Addr, const std::vector<DATA_TYPE>& argTypes, int line, int col);

    // ����� ���������� (�� �������) � ������ a � ������� ��������
    Tree* SemGetVar(const string& a, int line, int col);

    // ����� ������� � ������ a
    Tree* SemGetFunct(const string& a, int line, int col);

    // �������� ����� �� ������� ������ (���������� 1 � �����, 0 � ���)
    int DupControl(Tree* Addr, const string& a);

    // ����/����� �/�� ������� (��������� ��������)
    // SemEnterBlock ������ ��������� ���� ������� ��� Cur � ����������� Cur �� ����
    Tree* SemEnterBlock(int line, int col);
    void SemExitBlock();

    // ���������/��������� ������� �������
    static void SetCur(Tree* a) { Cur = a; }
    static Tree* GetCur() { return Cur; }

    // ������ ������ (�������)
    void Print(int level, const std::string& prefix, bool isLast);
    void Print(); // ���������� ������

private:
    // ��������������� ������ ������ � ���������
    static void SemError(const char* msg, const string& id = "", int line = -1, int col = -1);
};