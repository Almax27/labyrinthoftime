#pragma once
#include "pch.h"

class CellPoint {
public:
	int m_x, m_z;
	int previousState;
	CellPoint(int, int, int);
	~CellPoint();
};