#pragma once
#include "pch.h"
#include "Cell.h"
#include "CellPoint.h"
#include "AStar.h"


class Grid {
public:
	Cell cellMatrix[27][27];
	

	int getNeighbours(int x, int z);
	void initializeGrid();
	void nextGeneration();
	bool GetInitialised();
	void SetInitialised(bool state);
	void Clear();
	int Size();

	Cell GetTreasureCell();
	Cell GetPlayerCell();

	//AStar
	int* GetDistance();
	void ResetPlayerInStateMatrix(int,int);

	std::vector<CellPoint> haveChanged;

private:
	const int gridSize = 27;
	const int probabilityToBeAlive = 30;
	bool gridInitialised = false;
	bool m_isPlayerSet, m_isTreasureSet;
	
	Cell m_cellTreasure;
	Cell m_cellPlayer;

	AStar m_aStar;
	std::pair<int,int> m_treasureIndex;
	std::pair<int, int> m_playerIndex;
	int stateMatrix[27][27];
	bool m_LevelSolvable;
	bool m_distance;
	int m_searchResult;
};