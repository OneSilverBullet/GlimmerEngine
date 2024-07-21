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
#include "geometry/light.h"
#include "renderelement/renderitem.h"


class RootSignature;
class GraphicsPSO;
class Camera;

//todo: to build up render pass structure
class RenderScene
{
public:
	RenderScene();
	~RenderScene();
	void Initialize();
	void Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv,
		ColorBuffer& backBuffer, DepthBuffer& depthBuffer,
		D3D12_VIEWPORT viewport, D3D12_RECT scissorrect, DirectX::XMMATRIX& modelMat);
	void SetCamera(Camera* camera);

private:
	void InitializeRenderItems();
	void InitializeMaterials();
	void InitializeRootSignature();
	void InitializePSO();
	void InitializeLights();


private:
	//geometry part
	//todo: to build up render item structure

	std::vector<RenderItem> m_renderItems;
	//std::vector<ModelRef> m_renderItems;

	ByteAddressBuffer m_geometryBuffer;


	DescriptorHandle m_textureHandle;
	DescriptorHandle m_samplerHandle;





	DirectionLight m_dirLight;


	//root signature
	RootSignature* m_rootSignature;

	//pso
	GraphicsPSO* m_pso;

	//current camera
	Camera* m_camera = nullptr;
};
