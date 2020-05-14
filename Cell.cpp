#include "pch.h"
#include "Cell.h"


using namespace DirectX;

Cell::Cell() {
	cellState = 0;
	cellPosition = DirectX::SimpleMath::Vector3(0.f, 0.f, 0.f);
	cellDimensions = DirectX::SimpleMath::Vector3(1.f, 1.f, 1.f);
}

Cell::~Cell() {

}

void Cell::SetPosition(DirectX::SimpleMath::Vector3 position)
{
	cellPosition = position;
}

void Cell::SetCentre(DirectX::SimpleMath::Vector3 centre)
{
	cellCentre = centre; // pivot at centre in DX
}

DirectX::SimpleMath::Vector3 Cell::GetPosition()
{
	return cellPosition;
}

DirectX::SimpleMath::Vector3 Cell::GetDimensions() {
	return cellDimensions;
}

DirectX::SimpleMath::Vector3 Cell::GetCentre() {
	return cellCentre;
}

void Cell::SetState(int state)
{
	cellState = state;
}

int Cell::GetState()
{
	return cellState;
}

