#include "light.h"

using namespace DirectX;

void DirectionLight::InitializeLight(XMFLOAT3 dir, XMFLOAT3 col)
{
	m_direction = dir;
	SetColor(col);
}

void PointLight::InitializeLight(XMFLOAT3 pos, XMFLOAT3 col)
{
	m_position = pos;
	SetColor(col);
}
