#pragma once
#include <DirectXMath.h>


class LightBase
{
public:
	virtual void InitializeLight(DirectX::XMFLOAT3 var1, DirectX::XMFLOAT3 var2) = 0;
	virtual ~LightBase() {};
	void SetColor(DirectX::XMFLOAT3 color) { light_color = color; }
	DirectX::XMFLOAT3 GetColor() { return light_color; }
	virtual DirectX::XMFLOAT3 GetDirection() { return DirectX::XMFLOAT3(0, 0, 0); };
private:
	DirectX::XMFLOAT3 light_color; //the color of the light
};

class DirectionLight : public LightBase
{
public:
	void InitializeLight(DirectX::XMFLOAT3 dir, DirectX::XMFLOAT3 col);

	DirectX::XMFLOAT3 GetDirection() { return m_direction; }
	void SetDirection(DirectX::XMFLOAT3 dir) { m_direction = dir; }

private:
	DirectX::XMFLOAT3 m_direction;
};

class PointLight :public LightBase
{
public:
	void InitializeLight(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 col);

	DirectX::XMFLOAT3 GetPosition() { return m_position; }
	void SetPosition(DirectX::XMFLOAT3 pos) { m_position = pos; }
	void SetRadius(float v) { m_influenceRadius = v; }

private:
	DirectX::XMFLOAT3 m_position;
	float m_influenceRadius;
};

