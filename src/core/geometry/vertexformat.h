#pragma once

#include <string>
#include <DirectXMath.h>
#include <d3d12.h>


class BaseVertex
{
public:
	explicit BaseVertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT2 uv):
		position(pos), uv(uv){}
	explicit BaseVertex(float posX, float posY, float posZ, float uvU, float uvV) {
		position.x = posX;
		position.y = posY;
		position.z = posZ;
		uv.x = uvU;
		uv.y = uvV;
	}
	BaseVertex(const BaseVertex& bv) {
		position = bv.position;
		uv = bv.uv;
	}
	BaseVertex& operator=(const BaseVertex& v) {
		position = v.position;
		uv = v.uv;
		return *this;
	}

	//the core float3 vector
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 uv;
};

extern D3D12_INPUT_ELEMENT_DESC BaseVertexLayout[2];

