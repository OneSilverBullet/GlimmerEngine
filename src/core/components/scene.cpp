#include "scene.h"
#include "graphicscore.h"
#include "resources/colorbuffer.h"
#include "resources/depthbuffer.h"
#include "geometry/defaultgeometry.h"
#include "camera.h"

RenderScene::RenderScene() {

}

RenderScene::~RenderScene() {

}

void RenderScene::Initialize() {
    InitializeModels();
    InitializeMaterials();
    InitializeRootSignature();
    InitializePSO();
}

void RenderScene::SetCamera(Camera* camera) {
    m_camera = camera;
}

void RenderScene::Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv,
	ColorBuffer& backBuffer, DepthBuffer& depthBuffer,
	D3D12_VIEWPORT viewport, D3D12_RECT scissorrect, DirectX::XMMATRIX& modelMat) {

    assert(m_camera != nullptr);

    GraphicsContext& graphicsContext = GRAPHICS_CORE::g_contextManager.GetAvailableGraphicsContext();

    //change the back buffer's resource state
    {
        D3D12_RESOURCE_STATES state = backBuffer.GetUsageState();
        graphicsContext.TransitionResource(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
    }

    {
        graphicsContext.SetPiplelineObject(*m_pso);
        graphicsContext.SetRootSignature(*m_rootSignature);
        graphicsContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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
        __declspec(align(16)) struct CommonInfor
        {
            XMMATRIX model;
            XMMATRIX view;
            XMMATRIX proj;
            XMFLOAT3 eyepos;
        } commoninforcb;

        commoninforcb.model = modelMat;
        commoninforcb.view = m_camera->GetViewMatrix();
        commoninforcb.proj = m_camera->GetProjMatrix();
        commoninforcb.eyepos = m_camera->GetPosition();

        graphicsContext.SetDynamicConstantBufferView(0, sizeof(CommonInfor), &commoninforcb);
        D3D12_GPU_DESCRIPTOR_HANDLE texture_handle;
        texture_handle.ptr = m_textureHandle.GetGPUPtr();
        graphicsContext.SetDescriptorTable(1, texture_handle);
        D3D12_GPU_DESCRIPTOR_HANDLE sampler_handle;
        sampler_handle.ptr = m_samplerHandle.GetGPUPtr();
        graphicsContext.SetDescriptorTable(2, sampler_handle);

        for (int i = 0; i < m_renderItems.size(); ++i) {

            ModelRef modelRef = m_renderItems[i];
            
            std::vector<D3D12_VERTEX_BUFFER_VIEW>& submeshesVertices = modelRef.GetMeshVertexBufferView();
            std::vector<D3D12_INDEX_BUFFER_VIEW>& submeshesIndices = modelRef.GetIndicesVertexBufferView();
            std::vector<UINT32>& submeshesIndicesSizes = modelRef.GetIndicesSizes();

            for (int submeshIndex = 0; submeshIndex < submeshesVertices.size(); ++submeshIndex) {
                D3D12_VERTEX_BUFFER_VIEW subvertexView = submeshesVertices[submeshIndex];
                D3D12_INDEX_BUFFER_VIEW subindexView = submeshesIndices[submeshIndex];
                UINT32 subindicesSize = submeshesIndicesSizes[submeshIndex];

                graphicsContext.SetVertexBuffer(0, subvertexView);
                graphicsContext.SetIndexBuffer(subindexView);
                graphicsContext.DrawIndexedInstanced(subindicesSize, 1, 0, 0, 0);
            }
        }
    }

    // execute the sky box render pass
    {
        graphicsContext.TransitionResource(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, true);
        uint64_t fenceValue = graphicsContext.Finish(true);
    }

}

void RenderScene::InitializeModels() {
	std::string loadingModelPath = "resource\\models\\Cerberus.obj";
	ModelRef loadingModel = GRAPHICS_CORE::g_staticModelsManager.GetModelRef(loadingModelPath);
	m_renderItems.push_back(loadingModel);
}

void RenderScene::InitializeMaterials() {
    //loading texture
    m_testTexture = GRAPHICS_CORE::g_textureManager.LoadDDSFromFile("Cerberus/Cerberus_A.dds", BlackCubeMap, true);

    //allocate descriptor handle
    m_textureHandle = GRAPHICS_CORE::g_texturesDescriptorHeap.Alloc(1);
    m_samplerHandle = GRAPHICS_CORE::g_samplersDescriptorHeap.Alloc(1);

    //texture loading process
    D3D12_CPU_DESCRIPTOR_HANDLE textures[] = {
        m_testTexture.GetSRV()
    };

    UINT destNum = 1;
    UINT srcNums[] = { 1 };

    GRAPHICS_CORE::g_device->CopyDescriptors(1, &m_textureHandle, &destNum, destNum, textures, srcNums, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    //sampler loading process
    D3D12_CPU_DESCRIPTOR_HANDLE samplers[] = {
        GRAPHICS_CORE::g_samplerLinearWrap
    };

    GRAPHICS_CORE::g_device->CopyDescriptors(1, &m_samplerHandle, &destNum, destNum, 
        samplers, srcNums, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

void RenderScene::InitializeRootSignature() {
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

void RenderScene::InitializePSO() {
    //Load Shader
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"geometry_vertex.cso", &vertexShaderBlob));
    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"geometry_pixel.cso", &pixelShaderBlob));

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

    //Depth Stencil State
    D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
    depthStencilDesc.DepthEnable = TRUE;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencilDesc.StencilEnable = FALSE;

    //Create PSO
    m_pso = new GraphicsPSO();
    m_pso->SetNodeMask(0);
    m_pso->SetDepthStencilState(depthStencilDesc);
    m_pso->SetRasterizerState(rasterizerDesc);
    m_pso->SetBlendState(blendDesc);
    m_pso->SetRootSignature(m_rootSignature);
    m_pso->SetInputLayout(_countof(GeometryVertexLayout), GeometryVertexLayout);
    m_pso->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    m_pso->SetRenderTargetFormats(1, &rtvFormats.RTFormats[0], DXGI_FORMAT_D32_FLOAT);
    m_pso->SetVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
    m_pso->SetPixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
    m_pso->Finalize();
}