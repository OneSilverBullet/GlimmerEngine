
#include "game.h"
#include "window.h"
#include <DirectXMath.h>


class ClientGame : public Game
{
public:
	using super = Game;

	ClientGame(const std::wstring& name, int width, int height, bool vSync = false);
	
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
				Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
				ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
				size_t numElements, size_t elementSize, const void* bufferData,
				D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);

	void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

	void ResizeDepthBuffer(int width, int height);


	void ResizeBuffer(int width, int height);

	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	ComPtr<ID3D12Resource> m_depthBuffer; //samilar with the back buffer
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pso;

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	float m_fov;
	float m_aspectRatio;


	DirectX::XMMATRIX m_worldMatrix;
	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_projMatrix;

	bool m_contentLoaded;


	
};


