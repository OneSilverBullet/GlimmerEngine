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

//load hdr texture to cubemap
class HDRLoader
{
public:
	HDRLoader();
	~HDRLoader();
	void Initialize();
	void Render();
	TextureRef GetGeneratedCubemap() { return m_cubmapGenerated; }

private:
	void InitializeGeometry();
	void InitializeRootSignature();
	void InitializePSO();
	void InitializeHDRmap();
	void InitializeCubemapRenderTargets();


private:
	std::string m_cubemapName;

	UINT32 m_textureSize = 2048;

	//
	DXGI_FORMAT m_format = DXGI_FORMAT_R16G16B16A16_FLOAT; //DXGI_FORMAT_R8G8B8A8_UNORM_SRGB

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
	TextureRef m_hdrmap;
	TextureRef m_cubmapGenerated;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_cubemapRTV[6];

	//texture descriptor handles
	DescriptorHandle m_textureHandle;
	DescriptorHandle m_samplerHandle;
};



