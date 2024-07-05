#pragma once
#include <vector>
#include <string>
#include "geometry/vertexformat.h"
#include "resources/byteaddressbuffer.h"
#include "resources/colorbuffer.h"
#include "resources/depthbuffer.h"
#include "descriptortypes.h"
#include "texturemanager.h"


class RootSignature;
class GraphicsPSO;

class SkyBox
{
public:
	SkyBox();
	~SkyBox();
	void Initialize(std::string cubemapName);
	void Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv,
		ColorBuffer& backBuffer, DepthBuffer& depthBuffer,
		D3D12_VIEWPORT viewport, D3D12_RECT scissorrect,
		XMMATRIX& model, XMMATRIX& view, XMMATRIX& proj, XMFLOAT3& eyepos);

private:
	void InitializeGeometry();
	void InitializeRootSignature();
	void InitializePSO();
	void InitializeCubemap();

private:
	std::string m_cubemapName;

	//geometry part
	ByteAddressBuffer m_geometryBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	std::vector<PBRVertex> m_vertices;
	std::vector<DWORD> m_indicies;

	//root signature
	RootSignature* m_rootSignature;

	//pso
	GraphicsPSO* m_pso;

	//cubemap
	TextureRef m_cubemap;
	//texture descriptor handles
	DescriptorHandle m_textureHandle;
	DescriptorHandle m_samplerHandle;
};



