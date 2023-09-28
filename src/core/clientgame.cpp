
#include "clientgame.h"
#include "headers.h"
#include "application.h"
#include "commandqueue.h"

ClientGame::ClientGame(const std::wstring& name, int width, int height, bool vSync): 
	super(name, width, height, vSync)	
{



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
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backbuffer.Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        commandList->ResourceBarrier(1, &barrier);

        float green = 0.6f;

        FLOAT clearColor[4] = { 0.4f, green, 0.9f, 1.0f };

        commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }

    // Present
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backbuffer.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &barrier);

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
