#pragma once
#include "pch.h"
#include "Cell.h"
#include <iostream>
#include <queue>
#include <climits>
#include <cstring>
#include <stack>
#include <set>

#define ROW 27
#define COL 27

using namespace std;

// Creating a shortcut for int, int pair type 
typedef pair<int, int> Pair;

// Creating a shortcut for pair<int, pair<int, int>> type 
typedef pair<double, pair<int, int>> pPair;

class AStar {
public:
	int aStarSearch(int grid[][COL], Pair src, Pair dest);
	int* GetDistance();
	void ResetDistance();

private:
	void tracePath(Cell cellDetails[][COL], Pair dest);
	int distanceToPath = 0;
};