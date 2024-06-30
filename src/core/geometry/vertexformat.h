#pragma once

#include <string>
#include <DirectXMath.h>
#include <d3d12.h>

//use for skybox
class BaseVertex
{
public:
	explicit BaseVertex(DirectX::XMFLOAT3 pos) :
		position(pos) {}
	explicit BaseVertex(float posX, float posY, float posZ) {
		position.x = posX;
		position.y = posY;
		position.z = posZ;
	}
	BaseVertex(const BaseVertex& bv) {
		position = bv.position;
	}
	BaseVertex& operator=(const BaseVertex& v) {
		position = v.position;
		return *this;
	}

	DirectX::XMFLOAT3 position;
};

extern D3D12_INPUT_ELEMENT_DESC BaseVertexLayout[1];

//for pbr object
class PBRVertex
{
public:
	explicit PBRVertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 uv, DirectX::XMFLOAT3 normal) :
		position(pos), uv(uv), normal(normal) {}
	explicit PBRVertex(float posX, float posY, float posZ, float uvU, float uvV, float normalX, float normalY, float normalZ) {
		position.x = posX;
		position.y = posY;
		position.z = posZ;
		uv.x = uvU;
		uv.y = uvV;
		normal.x = normalX;
		normal.y = normalY;
		normal.z = normalZ;
	}
	PBRVertex(const PBRVertex& bv) {
		position = bv.position;
		uv = bv.uv;
		normal = bv.normal;
	}
	PBRVertex& operator=(const PBRVertex& v) {
		position = v.position;
		uv = v.uv;
		normal = v.normal;
		return *this;
	}

	//the core float3 vector
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 uv;
	DirectX::XMFLOAT3 normal;
};

extern D3D12_INPUT_ELEMENT_DESC PBRVertexLayout[3];

