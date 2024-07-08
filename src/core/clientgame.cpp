
#include "clientgame.h"
#include "headers.h"
#include "application.h"
#include "commandqueue.h"
#include "rootsignature.h"
#include "pso.h"
#include "graphicscore.h"
#include "d3dx12.h"
#include "context.h"
#include "resources/uploadbuffer.h"
#include "geometry/objloader.h"
#include "geometry/vertexformat.h"
#include "geometry/defaultgeometry.h"
#include <DirectXMath.h>
#include <fstream>
#include <iostream>
#include <string>

using namespace DirectX;


std::vector<PBRVertex> g_Vertices;
std::vector<DWORD> g_Indicies;

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

ClientGame::~ClientGame() {
    UnloadContent();
}

bool ClientGame::LoadContent() {
    auto device = GRAPHICS_CORE::g_device;

    //initialize the skybox
    m_skybox.Initialize("skybox");

    m_contentLoaded = true;
    ResizeDepthBuffer(GetClientWidth(), GetClientHeight());

	return true;
}

void ClientGame::UnloadContent() {
}

void ClientGame::OnRender(RenderEventArgs& e) {
    super::OnRender(e);
     
    ColorBuffer& currentBackbuffer = m_window->GetCurrentBackBuffer();
    auto rtv = m_window->GetCurrentRenderTargetView();
    auto dsv = m_depthBuffer.GetDSV();

    XMMATRIX mvpMatrix =  XMMatrixMultiply(XMMatrixMultiply(m_worldMatrix, m_viewMatrix), m_projMatrix);
    XMFLOAT3 eyepos = { 0, 0, -1 };


    //render sky box 
    m_skybox.Render(rtv, dsv, currentBackbuffer, m_depthBuffer, m_viewport, m_scissorRect, 
        m_worldMatrix, m_viewMatrix, m_projMatrix, eyepos);


    // Present
    {
        m_window->Present();
    }
}

void ClientGame::OnUpdate(UpdateEventArgs& e) {
    super::OnUpdate(e);

    //Update the model matrix
    float angle = static_cast<float>(e.TotalTime * 10.0f);
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 0, 0);
    m_worldMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

    //Update the view matrix
    const XMVECTOR eyePosition = XMVectorSet(0, 0, 0, 1);
    const XMVECTOR focusPoint = XMVectorSet(0, 0, 1, 1);
    const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
    m_viewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    //update project matrix
    float aspectRatio = GetClientWidth()/ static_cast<float>(GetClientHeight());
    m_projMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fov), aspectRatio, 0.1f, 1000.0f);

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
    if (e.Width != GetClientWidth() || e.Height != GetClientHeight()) {
        super::OnResize(e);
        m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(e.Width), static_cast<float>(e.Height));
        ResizeDepthBuffer(e.Width, e.Height);
    }
}

void ClientGame::OnWindowDestroy() {
	Application::GetInstance().Quit();
}

void ClientGame::ResizeDepthBuffer(int width, int height) {
    if (m_contentLoaded) {
        GRAPHICS_CORE::g_commandManager.Flush();
        width = std::max(1, width);
        height = std::max(1, height);
        auto device = GRAPHICS_CORE::g_device;

        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        optimizedClearValue.DepthStencil = { 1.0f, 0 };


        m_depthBuffer.Create(L"depthBuffer", width, height, D3D12_RESOURCE_STATE_DEPTH_WRITE, DXGI_FORMAT_D32_FLOAT);
    }
}