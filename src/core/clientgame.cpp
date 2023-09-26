
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
    auto commandListtmp = commandQueue->GetCommandList();
    
    UINT currentBackBufferIndex = m_window->GetCurrentBackBufferIndex();
    auto backbuffer = m_window->GetCurrentBackBuffer();
    auto rtv = m_window->GetCurrentRenderTargetView();

  

    ComPtr<ID3D12Resource> currentBackbuffer = m_window->GetCurrentBackBuffer();

    // Clear the render target.
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backbuffer.Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        commandListtmp->ResourceBarrier(1, &barrier);

        FLOAT clearColor[4] = { 0.4f, 0.6f, 0.9f, 1.0f };

        commandListtmp->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }

    // Present
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backbuffer.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandListtmp->ResourceBarrier(1, &barrier);

        ThrowIfFailed(commandListtmp->Close());


       UINT64 fenceValue = commandQueue->ExecuteCommandList(commandListtmp);


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
