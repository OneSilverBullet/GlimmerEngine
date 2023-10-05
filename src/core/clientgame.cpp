
#include "clientgame.h"
#include "headers.h"
#include "application.h"
#include "commandqueue.h"
#include "d3dx12.h"
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

bool ClientGame::LoadContent() {
    auto device = Application::GetInstance().GetDevice();
    auto commandQueue = Application::GetInstance().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->GetCommandList();
    //Upload vertex buffer data
    ComPtr<ID3D12Resource> intermediateVertexBuffer;
    UpdateBufferResource(commandList.Get(),
        &m_vertexBuffer, &intermediateVertexBuffer,
        _countof(g_Vertices), sizeof(VertexPosColor), g_Vertices);
    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.SizeInBytes = sizeof(g_Vertices);
    m_vertexBufferView.StrideInBytes = sizeof(VertexPosColor);

    //Upload index buffer data
    ComPtr<ID3D12Resource> intermediateIndexBuffer;
    UpdateBufferResource(commandList.Get(), 
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
    ComPtr<ID3DBlob> shaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"default.cso", &shaderBlob));


    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

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

    CD3DX12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlag);

    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
        		featureData.HighestVersion, &rootSignatureBlob, &errorBlob));

    ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
        				rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    //Create RTV
    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    PipelineStateStream pipelineStateStream;
    pipelineStateStream.p_rootSignature = m_rootSignature.Get();
    pipelineStateStream.p_inputLayout = { inputLayout, _countof(inputLayout) };
    pipelineStateStream.p_primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.p_vs = CD3DX12_SHADER_BYTECODE(shaderBlob.Get());
    pipelineStateStream.p_ps = CD3DX12_SHADER_BYTECODE(shaderBlob.Get());
    pipelineStateStream.p_dsvFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.p_rtvFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(pipelineStateStream), &pipelineStateStream
	};
    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pso)));

    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaiteForFenceValue(fenceValue);
    m_contentLoaded = true;

    Res(GetClientWidth(), GetClientHeight());

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

void ClientGame::ResizeDepthBuffer(int width, int height) {
    if (m_contentLoaded) {
        Application::GetInstance().Flush();
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
    

        D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
		dsv.Format = DXGI_FORMAT_D32_FLOAT;
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0;
		dsv.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(m_depthBuffer.Get(), &dsv, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        
    }
}

void ClientGame::ResizeBuffer(int width, int height) {



}
