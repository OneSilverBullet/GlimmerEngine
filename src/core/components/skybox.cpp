#include "skybox.h"
#include "graphicscore.h"
#include "geometry/defaultgeometry.h"
#include "resources/uploadbuffer.h"
#include "rootsignature.h"
#include "pso.h"
#include "d3dx12.h"
#include <wrl/client.h>

SkyBox::SkyBox()
{

}

SkyBox::~SkyBox() {

}

void SkyBox::InitializeGeometry() {

    DefaultGeometry::DefaultSphereMesh(40.0f, m_vertices, m_indicies);

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

void SkyBox::InitializeRootSignature() {
    //Create root signature
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(GRAPHICS_CORE::g_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
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

void SkyBox::InitializePSO() {
    //Load Shader
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"base_vertex.cso", &vertexShaderBlob));
    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"default_pixel.cso", &pixelShaderBlob));

    //Create RTV
    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

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

void SkyBox::InitializeCubemap() {
    //generate texture
    //m_testTextureRef = GRAPHICS_CORE::g_textureManager.LoadDDSFromFile("spnza_bricks_a", WhiteOpaque2D, true);
    m_cubemap = GRAPHICS_CORE::g_textureManager.LoadDDSFromFile(m_cubemapName, BlackCubeMap, true);

    //allocate descriptor handle
    m_textureHandle = GRAPHICS_CORE::g_texturesDescriptorHeap.Alloc(1);
    m_samplerHandle = GRAPHICS_CORE::g_samplersDescriptorHeap.Alloc(1);

    //texture loading process
    D3D12_CPU_DESCRIPTOR_HANDLE textures[] = {
        m_cubemap.GetSRV()
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

void SkyBox::Initialize(std::string skyboxName) {
    m_cubemapName = skyboxName;
    InitializeGeometry();
    InitializeRootSignature();
    InitializePSO();
    InitializeCubemap();
}

void SkyBox::Render(
    D3D12_CPU_DESCRIPTOR_HANDLE rtv, 
    D3D12_CPU_DESCRIPTOR_HANDLE dsv,
    ColorBuffer& backbuffer,
    DepthBuffer& depthbuffer,
    D3D12_VIEWPORT viewport,
    D3D12_RECT scissorrect,
    XMMATRIX& model, XMMATRIX& view, XMMATRIX& proj,
    XMFLOAT3& eyepos) {

    GraphicsContext& graphicsContext = GRAPHICS_CORE::g_contextManager.GetAvailableGraphicsContext();

    // Clear the render target.
    {
        D3D12_RESOURCE_STATES state = backbuffer.GetUsageState();
        graphicsContext.TransitionResource(backbuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
        FLOAT clearColor[4] = { 0.4f, 0.6f, 0.9f, 1.0f };
        graphicsContext.ClearColor(rtv, clearColor);
        graphicsContext.ClearDepth(depthbuffer);
    }

    {
        graphicsContext.SetPiplelineObject(*m_pso);
        graphicsContext.SetRootSignature(*m_rootSignature);
        graphicsContext.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        graphicsContext.SetVertexBuffer(0, m_vertexBufferView);
        graphicsContext.SetIndexBuffer(m_indexBufferView);
        graphicsContext.SetViewportAndScissor(viewport, scissorrect);
        graphicsContext.SetRenderTargets(1, &rtv, dsv);
    }

    //set the descriptor heap
    {
        graphicsContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, GRAPHICS_CORE::g_texturesDescriptorHeap.GetDescriptorHeap());
        graphicsContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, GRAPHICS_CORE::g_samplersDescriptorHeap.GetDescriptorHeap());
    }

    //bind the shader visible resource
    {
        __declspec(align(16)) struct SkyboxCB
        {
            XMMATRIX model;
            XMMATRIX view;
            XMMATRIX proj;
            XMFLOAT3 eyepos;
        } skyboxcbuffer;
        
        skyboxcbuffer.model = model;
        skyboxcbuffer.view = view;
        skyboxcbuffer.proj = proj;
        skyboxcbuffer.eyepos = eyepos;

        graphicsContext.SetDynamicConstantBufferView(0, sizeof(SkyboxCB), &skyboxcbuffer);
        graphicsContext.SetDescriptorTable(1, GRAPHICS_CORE::g_texturesDescriptorHeap.GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
        graphicsContext.SetDescriptorTable(2, GRAPHICS_CORE::g_samplersDescriptorHeap.GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
        graphicsContext.DrawIndexedInstanced(m_indicies.size(), 1, 0, 0, 0);
    }

    // execute the sky box render pass
    {
        graphicsContext.TransitionResource(backbuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, true);
        uint64_t fenceValue = graphicsContext.Finish(true);
    }
}
