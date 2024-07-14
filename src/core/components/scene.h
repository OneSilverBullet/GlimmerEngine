#pragma once
#include <vector>
#include <string>
#include "geometry/vertexformat.h"
#include "geometry/model.h"
#include "resources/byteaddressbuffer.h"
#include "resources/colorbuffer.h"
#include "resources/depthbuffer.h"
#include "descriptortypes.h"
#include "texturemanager.h"
#include "components/hdrtocubemap.h"

class RootSignature;
class GraphicsPSO;


class RenderScene
{
public:
	RenderScene();
	~RenderScene();
	void Initialize();
	void Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv,
		ColorBuffer& backBuffer, DepthBuffer& depthBuffer,
		D3D12_VIEWPORT viewport, D3D12_RECT scissorrect);

private:
	void InitializeModels();
	void InitializeMaterials();
	void InitializeRootSignature();
	void InitializePSO();


private:
	//geometry part

	//todo: materials loading; materials manager

	//root signature
	RootSignature* m_rootSignature;

	//pso
	GraphicsPSO* m_pso;
};
