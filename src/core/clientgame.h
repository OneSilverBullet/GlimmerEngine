
#include "game.h"
#include "window.h"
#include <DirectXMath.h>
#include "resources/depthbuffer.h"

class RootSignature;
class GraphicsPSO;


class ClientGame : public Game
{
public:
	using super = Game;

	ClientGame(const std::wstring& name, int width, int height, bool vSync = false);
	virtual ~ClientGame();
	virtual bool LoadContent() override;
	virtual void UnloadContent() override;

protected:

	virtual void OnUpdate(UpdateEventArgs& e) override;
	virtual void OnRender(RenderEventArgs& e) override;
	virtual void OnKeyPressed(KeyEventArgs& e) override;
	virtual void OnKeyReleased(KeyEventArgs& e) override;
	virtual void OnMouseMoved(MouseMotionEventArgs& e) override;
	virtual void OnMouseWheel(MouseWheelEventArgs& e) override;
	virtual void OnResize(ResizeEventArgs& e) override;
	virtual void OnWindowDestroy() override;

private:
	void UpdateBufferResource(
				ID3D12GraphicsCommandList* commandList,
				ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
				size_t numElements, size_t elementSize, const void* bufferData,
				D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	void TransitionResource(ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* resource,
		D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	void ClearRTV(ID3D12GraphicsCommandList* commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);

	void ClearDepth(ID3D12GraphicsCommandList* commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);


	void ResizeDepthBuffer(int width, int height);


	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;


	DepthBuffer m_depthBuffer;

	//ComPtr<ID3D12Resource> m_depthBuffer; //samilar with the back buffer
	//D3D12_CPU_DESCRIPTOR_HANDLE m_dsvDescriptorHandle;

	RootSignature* m_rootSignature = nullptr;
	GraphicsPSO* m_pso = nullptr;

	//the command is temporary
	//todo: encapsulate a graphics context
	ID3D12GraphicsCommandList* m_commandList = nullptr;

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	float m_fov;
	float m_aspectRatio;

	DirectX::XMMATRIX m_worldMatrix;
	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_projMatrix;

	bool m_contentLoaded;
};


