
#include "clientgame.h"
#include "headers.h"
#include "application.h"
#include "commandqueue.h"

#include <DirectXMath.h>

using namespace DirectX;

struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};

static VertexPosColor g_Vertices[8] = {
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
    { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
    { XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
    { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
    { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
    { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
    { XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
    { XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

static WORD g_Indicies[36] =
{
    0, 1, 2, 0, 2, 3,
    4, 6, 5, 4, 7, 6,
    4, 5, 1, 4, 1, 0,
    3, 2, 6, 3, 6, 7,
    1, 5, 6, 1, 6, 2,
    4, 0, 3, 4, 3, 7
};

template<typename T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi)
{
	return v < lo ? lo : hi < v ? hi : v;
}

ClientGame::ClientGame(const std::wstring& name, int width, int height, bool vSync): 
	super(name, width, height, vSync)	
{
    m_scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
    m_fov = 45.0f;
    m_contentLoaded = false;

}

bool ClientGame::LoadContent() {

	return true;
}

void ClientGame::UnloadContent() {

}

void ClientGame::OnRender(RenderEventArgs& e) {
    super::OnRender(e);


    
    auto commandQueue = Application::GetInstance().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();
    

    UINT currentBackBufferIndex = m_window->GetCurrentBackBufferIndex();
    auto backbuffer = m_window->GetCurrentBackBuffer();
    auto rtv = m_window->GetCurrentRenderTargetView();

  

    ComPtr<ID3D12Resource> currentBackbuffer = m_window->GetCurrentBackBuffer();

    // Clear the render target.
    {
        TransitionResource(commandList, currentBackbuffer,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        float green = 0.6f;

        FLOAT clearColor[4] = { 0.4f, green, 0.9f, 1.0f };

        commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }

    // Present
    {
        TransitionResource(commandList, currentBackbuffer,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        ThrowIfFailed(commandList->Close());


       UINT64 fenceValue = commandQueue->ExecuteCommandList(commandList);


       currentBackBufferIndex = m_window->Present();

       commandQueue->WaiteForFenceValue(fenceValue);
    }
}

void ClientGame::OnUpdate(UpdateEventArgs& e) {

}

void ClientGame::OnKeyPressed(KeyEventArgs& e) {
	


}

void ClientGame::OnKeyReleased(KeyEventArgs& e) {

}

void ClientGame::OnMouseMoved(MouseMotionEventArgs& e) {

}

void ClientGame::OnMouseWheel(MouseWheelEventArgs& e) {

}

void ClientGame::OnResize(ResizeEventArgs& e) {

}

void ClientGame::OnWindowDestroy() {
	Application::GetInstance().Quit();
}


void ClientGame::TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
    Microsoft::WRL::ComPtr<ID3D12Resource> resource,
    D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState) {

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource.Get(),
        beforeState, afterState);

    commandList->ResourceBarrier(1, &barrier);
}

void ClientGame::ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor) {
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void ClientGame::ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f) {
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void ClientGame::ResizeBuffer(int width, int height) {

}
