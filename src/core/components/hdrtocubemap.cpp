#include "skybox.h"
#include "graphicscore.h"
#include "geometry/defaultgeometry.h"
#include "resources/uploadbuffer.h"

#include "rootsignature.h"
#include "pso.h"
#include "d3dx12.h"
#include <wrl/client.h>
#include "hdrtocubemap.h"


HDRLoader::HDRLoader() {

}

HDRLoader::~HDRLoader() {

}

void HDRLoader::Initialize() {
    InitializeGeometry();
    InitializeRootSignature();
    InitializePSO();
    InitializeHDRmap();
    InitializeCubemapRenderTargets();
}

void HDRLoader::Render() {

    D3D12_RECT scissorrect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    D3D12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, (float)m_textureSize, (float)m_textureSize);

    __declspec(align(16)) struct HDRLoaderCB
    {
        XMMATRIX model;
        XMMATRIX view;
        XMMATRIX proj;
    } hdrloadercbuffer;

    //update project matrix
    XMMATRIX model = XMMatrixIdentity();
    XMMATRIX projMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(90.0f), 1.0f, 0.1f, 1000.0f);
    XMMATRIX viewMatrix[] =
    {
         XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), XMVectorSet(1.0f,  0.0f,  0.0f, 1.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)),
         XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), XMVectorSet(-1.0f,  0.0f,  0.0f, 1.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)),
         XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), XMVectorSet(0.0f,  1.0f,  0.0f, 1.0f), XMVectorSet(0.0f,  0.0f,  -1.0f, 0.0f)),
         XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), XMVectorSet(0.0f,  -1.0f,  0.0f, 1.0f), XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)),
         XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), XMVectorSet(0.0f,  0.0f,  1.0f, 1.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)),
         XMMatrixLookAtLH(XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f), XMVectorSet(0.0f,  0.0f, -1.0f, 1.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)),
    };

    GraphicsContext& graphicsContext = GRAPHICS_CORE::g_contextManager.GetAvailableGraphicsContext();


    //initialize the graphics context
    graphicsContext.SetPiplelineObject(*m_pso);
    graphicsContext.SetRootSignature(*m_rootSignature);
    graphicsContext.SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    graphicsContext.SetVertexBuffer(0, m_vertexBufferView);
    graphicsContext.SetIndexBuffer(m_indexBufferView);
    graphicsContext.SetViewportAndScissor(viewport, scissorrect);
    graphicsContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, GRAPHICS_CORE::g_texturesDescriptorHeap.GetDescriptorHeap());
    graphicsContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, GRAPHICS_CORE::g_samplersDescriptorHeap.GetDescriptorHeap());

    for (int i = 0; i < 6; ++i) {
        //clear the render target
        graphicsContext.SetRenderTargets(1, &m_cubemapRTV[i]);
        FLOAT clearColor[4] = { 0.4f, 0.6f, 0.9f, 1.0f };
        graphicsContext.ClearColor(m_cubemapRTV[i], clearColor);


        hdrloadercbuffer.model = model;
        hdrloadercbuffer.view = viewMatrix[i];
        hdrloadercbuffer.proj = projMatrix;
        

        graphicsContext.SetDynamicConstantBufferView(0, sizeof(HDRLoaderCB), &hdrloadercbuffer);
        D3D12_GPU_DESCRIPTOR_HANDLE texture_handle;
        texture_handle.ptr = m_textureHandle.GetGPUPtr();
        graphicsContext.SetDescriptorTable(1, texture_handle);
        D3D12_GPU_DESCRIPTOR_HANDLE sampler_handle;
        sampler_handle.ptr = m_samplerHandle.GetGPUPtr();
        graphicsContext.SetDescriptorTable(2, sampler_handle);
        graphicsContext.DrawIndexedInstanced(m_indicies.size(), 1, 0, 0, 0);
    }

    uint64_t fenceValue = graphicsContext.Finish(true);
}

void HDRLoader::InitializeGeometry()
{
    DefaultGeometry::DefaultBoxMesh(0.5f, m_vertices, m_indicies);

    uint32_t vertexOffset = m_vertices.size() * sizeof(PBRVertex);
    uint32_t indexOffset = m_indicies.size() * sizeof(DWORD);
    uint32_t uploadBufferSize = m_vertices.size() * sizeof(PBRVertex) + m_indicies.size() * sizeof(DWORD);

    UploadBuffer uploadBuffer;
    uploadBuffer.Create(L"Upload Buffer", uploadBufferSize);

    uint8_t* uploadMem = (uint8_t*)uploadBuffer.Map();

    memcpy(uploadMem, m_vertices.data(), vertexOffset);
    memcpy(uploadMem + vertexOffset, m_indicies.data(), indexOffset);

    m_geometryBuffer.Create(L"Geometry Buffer", uploadBufferSize, 1, uploadBuffer);

    m_vertexBufferView = m_geometryBuffer.VertexBufferView(0, vertexOffset, sizeof(PBRVertex));
    m_indexBufferView = m_geometryBuffer.IndexBufferView(vertexOffset, indexOffset, true);
}

void HDRLoader::InitializeRootSignature()
{
    //Create root signature
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(GRAPHICS_CORE::g_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE,
        &featureData, sizeof(featureData)))) {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlag =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    m_rootSignature = new RootSignature(3, 0);
    (*m_rootSignature)[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
    (*m_rootSignature)[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
    (*m_rootSignature)[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 1, D3D12_SHADER_VISIBILITY_ALL);
    m_rootSignature->Finalize(L"", rootSignatureFlag);
}

void HDRLoader::InitializePSO()
{
    //Load Shader
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"equirectangular_vertex.cso", &vertexShaderBlob));
    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"equirectangular_pixel.cso", &pixelShaderBlob));

    //Create RTV
    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = m_format;

    //Create Rasterizer State
    D3D12_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
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
    m_pso->SetInputLayout(_countof(PBRVertexLayout), PBRVertexLayout);
    m_pso->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    m_pso->SetRenderTargetFormats(1, &rtvFormats.RTFormats[0], DXGI_FORMAT_D32_FLOAT);
    m_pso->SetVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
    m_pso->SetPixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
    m_pso->Finalize();
}

void HDRLoader::InitializeCubemapRenderTargets() {
    

    ManagedTexture* texInstance = nullptr;
    texInstance = new ManagedTexture("file");

    UINT formatSize = GRAPHICS_CORE::GetDXGIFormatSize(m_format);

    uint32_t* formattedData = new uint32_t[formatSize * m_textureSize * m_textureSize];
    texInstance->CreateCube(formatSize * m_textureSize, m_textureSize, m_textureSize, m_format,
        formattedData, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_cubmapGenerated = TextureRef(texInstance);

    D3D12_CPU_DESCRIPTOR_HANDLE cubeRTVHandles = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 6);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
    rtvDesc.Format = m_format;
    rtvDesc.Texture2DArray.MipSlice = 0;
    rtvDesc.Texture2DArray.FirstArraySlice = 0; 
    rtvDesc.Texture2DArray.ArraySize = 1;
    rtvDesc.Texture2DArray.PlaneSlice = 0;

    UINT32 descriptorSize = GRAPHICS_CORE::GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (int cubemapIndex = 0; cubemapIndex < 6; ++cubemapIndex) {
        D3D12_CPU_DESCRIPTOR_HANDLE tmpRTVHandle = cubeRTVHandles;
        tmpRTVHandle.ptr = cubeRTVHandles.ptr + cubemapIndex * descriptorSize;
        m_cubemapRTV[cubemapIndex] = CD3DX12_CPU_DESCRIPTOR_HANDLE(tmpRTVHandle);
        //change the render target of the cube map
        rtvDesc.Texture2DArray.FirstArraySlice = cubemapIndex;
        GRAPHICS_CORE::g_device->CreateRenderTargetView(m_cubmapGenerated.Get()->GetResource(), &rtvDesc, m_cubemapRTV[cubemapIndex]);
    }
}

void HDRLoader::InitializeHDRmap()
{
    //loading hdr texture
    m_hdrmap = GRAPHICS_CORE::g_textureManager.LoadDDSFromFile("hdr/hdrsky.hdr", BlackCubeMap, true);

    //allocate descriptor handle
    m_textureHandle = GRAPHICS_CORE::g_texturesDescriptorHeap.Alloc(1);
    m_samplerHandle = GRAPHICS_CORE::g_samplersDescriptorHeap.Alloc(1);

    //texture loading process
    D3D12_CPU_DESCRIPTOR_HANDLE textures[] = {
        m_hdrmap.GetSRV()
    };

    UINT destNum = 1;
    UINT srcNums[] = { 1 };

    GRAPHICS_CORE::g_device->CopyDescriptors(1, &m_textureHandle, &destNum, destNum, textures, srcNums, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    //sampler loading process
    D3D12_CPU_DESCRIPTOR_HANDLE samplers[] = {
        GRAPHICS_CORE::g_samplerLinearWrap
    };

    GRAPHICS_CORE::g_device->CopyDescriptors(1, &m_samplerHandle, &destNum, destNum, samplers, srcNums, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}
