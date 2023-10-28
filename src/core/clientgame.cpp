
#include "clientgame.h"
#include "headers.h"
#include "application.h"
#include "commandqueue.h"
#include "rootsignature.h"
#include "pso.h"
#include "graphicscore.h"
#include "d3dx12.h"
#include <DirectXMath.h>
#include <fstream>
#include <iostream>
#include <string>

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

struct PipelineStateStream
{
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE p_rootSignature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT p_inputLayout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY p_primitiveTopologyType;
    CD3DX12_PIPELINE_STATE_STREAM_VS p_vs;
    CD3DX12_PIPELINE_STATE_STREAM_PS p_ps;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT p_dsvFormat;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS p_rtvFormats;
};


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
    auto device = Application::GetInstance().GetDevice();

    CommandQueue& commandQueue = GRAPHICS_CORE::g_commandManager.GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

    //load content, the first time to initialize the command list
    ID3D12CommandAllocator* commandAllocator = nullptr;
    GRAPHICS_CORE::g_commandManager.CreateNewCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, &m_commandList, &commandAllocator);

    //Upload vertex buffer data
    ComPtr<ID3D12Resource> intermediateVertexBuffer;
    UpdateBufferResource(m_commandList,
        &m_vertexBuffer, &intermediateVertexBuffer,
        _countof(g_Vertices), sizeof(VertexPosColor), g_Vertices);
    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.SizeInBytes = sizeof(g_Vertices);
    m_vertexBufferView.StrideInBytes = sizeof(VertexPosColor);

    //Upload index buffer data
    ComPtr<ID3D12Resource> intermediateIndexBuffer;
    UpdateBufferResource(m_commandList,
        &m_indexBuffer, &intermediateIndexBuffer,
        		_countof(g_Indicies), sizeof(WORD), g_Indicies);
    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_indexBufferView.SizeInBytes = sizeof(g_Indicies);

    //Create the depth descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

    //Load Shader
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"base_vertex.cso", &vertexShaderBlob));
    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"default_pixel.cso", &pixelShaderBlob));

    //Create root signature
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlag = 
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    m_rootSignature = new RootSignature(1, 0);
    (*m_rootSignature)[0].InitAsConstant32(0, sizeof(XMMATRIX) / 4, D3D12_SHADER_VISIBILITY_VERTEX);
    m_rootSignature->Finalize(L"", rootSignatureFlag);

    //Create RTV
    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    //Create input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR_IN",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    //Create Rasterizer State
    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FrontCounterClockwise = FALSE;
    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizerDesc.DepthClipEnable = TRUE;

    //Create Blend State
    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = FALSE;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;


    //Create PSO
    m_pso = new GraphicsPSO();
    m_pso->SetNodeMask(0);
    m_pso->SetRasterizerState(rasterizerDesc);
    m_pso->SetBlendState(blendDesc);
    m_pso->SetRootSignature(m_rootSignature);
    m_pso->SetInputLayout(_countof(inputLayout), inputLayout);
    m_pso->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    m_pso->SetRenderTargetFormats(1, &rtvFormats.RTFormats[0], DXGI_FORMAT_D32_FLOAT);
    m_pso->SetVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
    m_pso->SetPixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
    m_pso->Finalize();

    ThrowIfFailed(m_commandList->Close());
    auto fenceValue = commandQueue.ExecuteCommandList((ID3D12GraphicsCommandList2*)m_commandList);
    commandQueue.WaitForFence(fenceValue);
    m_contentLoaded = true;
    ResizeDepthBuffer(GetClientWidth(), GetClientHeight());

    //reload the allocator
    GRAPHICS_CORE::g_commandManager.ReallocateCommandAllocator(fenceValue, commandAllocator);
	return true;
}

void ClientGame::UnloadContent() {
    if (m_commandList != nullptr) {
        m_commandList->Release();
    }
}

void ClientGame::OnRender(RenderEventArgs& e) {
    super::OnRender(e);

    CommandQueue& commandQueue = GRAPHICS_CORE::g_commandManager.GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    ID3D12CommandAllocator* commandAllocator = nullptr;
    GRAPHICS_CORE::g_commandManager.ResetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, &m_commandList, &commandAllocator);

    
    UINT currentBackBufferIndex = m_window->GetCurrentBackBufferIndex();
    auto backbuffer = m_window->GetCurrentBackBuffer();
    auto rtv = m_window->GetCurrentRenderTargetView();
    auto dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
  

    ComPtr<ID3D12Resource> currentBackbuffer = m_window->GetCurrentBackBuffer();

    // Clear the render target.
    {
        TransitionResource(m_commandList, currentBackbuffer,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        FLOAT clearColor[4] = { 0.4f, 0.6f, 0.9f, 1.0f };

        ClearRTV(m_commandList, rtv, clearColor);
        ClearDepth(m_commandList, dsv);
    }

    //Assemble the rendering components
    {
        m_commandList->SetPipelineState(m_pso->GetPSO());
        m_commandList->SetGraphicsRootSignature(m_rootSignature->GetSignature());

        //Assemble Geometry
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        m_commandList->IASetIndexBuffer(&m_indexBufferView);

        //Setup Rasterizer State
        m_commandList->RSSetViewports(1, &m_viewport);
        m_commandList->RSSetScissorRects(1, &m_scissorRect);
        
        //Bind Render target
        m_commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

        //Update Constant buffer
        XMMATRIX mvpMatrix = XMMatrixMultiply(XMMatrixMultiply(m_worldMatrix, m_viewMatrix), m_projMatrix);
        m_commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

        XMFLOAT4X4 mvp;
        XMStoreFloat4x4(&mvp, mvpMatrix);
        std::ofstream fout;
        fout.open("mvp.txt");
        fout << std::to_string(mvp._11) << " " << std::to_string(mvp._12) << " " << std::to_string(mvp._13) << " " << std::to_string(mvp._14) << std::endl
            << std::to_string(mvp._21) << " " << std::to_string(mvp._22) << " " << std::to_string(mvp._23) << " " << std::to_string(mvp._24) << std::endl
            << std::to_string(mvp._31) << " " << std::to_string(mvp._32) << " " << std::to_string(mvp._33) << " " << std::to_string(mvp._34) << std::endl
            << std::to_string(mvp._41) << " " << std::to_string(mvp._42) << " " << std::to_string(mvp._43) << " " << std::to_string(mvp._44) << std::endl;


        m_commandList->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);
    }

    // Present
    {
        TransitionResource(m_commandList, currentBackbuffer,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        ThrowIfFailed(m_commandList->Close());
        uint64_t fenceValue = commandQueue.ExecuteCommandList((ID3D12GraphicsCommandList2*)m_commandList);
        currentBackBufferIndex = m_window->Present();
        commandQueue.WaitForFence(fenceValue);

        //reallocate allocator
        GRAPHICS_CORE::g_commandManager.ReallocateCommandAllocator(fenceValue, commandAllocator);
    }
}

void ClientGame::OnUpdate(UpdateEventArgs& e) {
    super::OnUpdate(e);

    //Update the model matrix
    float angle = static_cast<float>(e.TotalTime * 90.0f);
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
    m_worldMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

    //Update the view matrix
    const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
    const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
    const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
    m_viewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    //update project matrix
    float aspectRatio = GetClientWidth()/ static_cast<float>(GetClientHeight());
    m_projMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fov), aspectRatio, 0.1f, 100.0f);

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

void ClientGame::UpdateBufferResource(
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
    ID3D12Resource** pDestinationResource, //destination resource
    ID3D12Resource** pIntermediateResource, //intermediate resource
    size_t numElements, size_t elementSize, const void* bufferData,
    D3D12_RESOURCE_FLAGS flags) {

    auto device = Application::GetInstance().GetDevice();
    size_t bufferSize = numElements * elementSize;

    // Create a committed resource for the GPU resource in a default heap.
    ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
			D3D12_HEAP_FLAG_NONE, // no flags
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags), // resource description for a buffer
			D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy destination state since we will copy data
			nullptr, // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
			IID_PPV_ARGS(pDestinationResource)));

    if (bufferData != nullptr) {
        ThrowIfFailed(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
            D3D12_HEAP_FLAG_NONE, // no flags
            &CD3DX12_RESOURCE_DESC::Buffer(bufferSize), // resource description for a buffer
            D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
            nullptr,
            IID_PPV_ARGS(pIntermediateResource)));
        
        //Upload the buffer data to the GPU
        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = bufferData;
        subresourceData.RowPitch = bufferSize;
        subresourceData.SlicePitch = subresourceData.RowPitch;
        UpdateSubresources(commandList.Get(), 
            *pDestinationResource, *pIntermediateResource,
            0, 0, 1, &subresourceData);
    }
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
    D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth) {
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void ClientGame::TransitionResource(ID3D12GraphicsCommandList* commandList,
    Microsoft::WRL::ComPtr<ID3D12Resource> resource,
    D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState) {
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource.Get(),
        beforeState, afterState);
    commandList->ResourceBarrier(1, &barrier);
}


void ClientGame::ClearRTV(ID3D12GraphicsCommandList* commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor) {
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void ClientGame::ClearDepth(ID3D12GraphicsCommandList* commandList,
    D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth) {
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void ClientGame::ResizeDepthBuffer(int width, int height) {
    if (m_contentLoaded) {
        GRAPHICS_CORE::g_commandManager.Flush();
        width = std::max(1, width);
        height = std::max(1, height);
        auto device = Application::GetInstance().GetDevice();

        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        optimizedClearValue.DepthStencil = { 1.0f, 0 };

        ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
			D3D12_HEAP_FLAG_NONE, // no flags
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
                				1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL), // resource description for a depth buffer
			D3D12_RESOURCE_STATE_DEPTH_WRITE, // we will start this heap in the generic read state as a copy destination
			&optimizedClearValue, // the optimized clear value for depth buffers
			IID_PPV_ARGS(&m_depthBuffer)));
    
        // 
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
		dsv.Format = DXGI_FORMAT_D32_FLOAT;
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0;
		dsv.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(m_depthBuffer.Get(), &dsv, 
            m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    }
}