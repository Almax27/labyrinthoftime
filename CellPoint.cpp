#include "pch.h"
#include "CellPoint.h"

CellPoint::CellPoint(int x, int z, int state)
{
	m_x = x;
	m_z = z;
	previousState = state;
}

CellPoint::~CellPoint()
{
}
