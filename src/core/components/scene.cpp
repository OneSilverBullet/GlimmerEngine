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
    InitializeRenderItems();
    InitializeMaterials();
    InitializeRootSignature();
    InitializePSO();
    InitializeLights();
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
            XMFLOAT3 sundirection;
            XMFLOAT3 sunintensity;
            XMFLOAT2 iblparameter; //[0] is ibl range, [1] is ibl bias
        } commoninforcb;

        commoninforcb.model = modelMat;
        commoninforcb.view = m_camera->GetViewMatrix();
        commoninforcb.proj = m_camera->GetProjMatrix();
        commoninforcb.eyepos = m_camera->GetPosition();
        commoninforcb.sundirection = m_dirLight.GetDirection();
        commoninforcb.sunintensity = m_dirLight.GetColor();
        commoninforcb.iblparameter = XMFLOAT2(0.0F, 0.0F);
        graphicsContext.SetDynamicConstantBufferView(0, sizeof(CommonInfor), &commoninforcb);

        //for each model
        for (int i = 0; i < m_renderItems.size(); ++i) {
            RenderItem renderItem = m_renderItems[i];
            UINT submeshSize = m_renderItems[i].GetSubmeshSize();

            std::vector<D3D12_VERTEX_BUFFER_VIEW>& submeshesVertices = renderItem.GetMeshVertexBufferView();
            std::vector<D3D12_INDEX_BUFFER_VIEW>& submeshesIndices = renderItem.GetIndicesVertexBufferView();
            std::vector<UINT32>&  submeshesIndicesSizes = renderItem.GetIndicesSizes();
            std::vector<uint16_t>& submeshesTexturesSRV = renderItem.GetTextureSRVOffset();
            std::vector<uint16_t>& submeshesSamplersSRV = renderItem.GetSamplersSRVOffset();

            //for each submesh in model
            for (int submeshIndex = 0; submeshIndex < submeshSize; ++submeshIndex) {
                D3D12_VERTEX_BUFFER_VIEW subvertexView = submeshesVertices[submeshIndex];
                D3D12_INDEX_BUFFER_VIEW subindexView = submeshesIndices[submeshIndex];
                UINT32 subindicesSize = submeshesIndicesSizes[submeshIndex];
                //the srv related resources
                uint16_t textureSRV = submeshesTexturesSRV[submeshIndex];
                uint16_t samplerSRV = submeshesSamplersSRV[submeshIndex];

                graphicsContext.SetDescriptorTable(1, GRAPHICS_CORE::g_texturesDescriptorHeap[textureSRV]);
                graphicsContext.SetDescriptorTable(2, GRAPHICS_CORE::g_samplersDescriptorHeap[samplerSRV]);
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

void RenderScene::InitializeRenderItems() {
    //todo: merge the same render items and merge batch algorithm
    
    RenderItem renderItem;
    renderItem.Initialize("Cerberus");

    m_renderItems.push_back(renderItem);
}

void RenderScene::InitializeMaterials() {

    for (int i = 0; i < (int)m_renderItems.size(); ++i) {
        std::vector<Material*> renderItemMaterials = m_renderItems[i].GetMaterials();
        for (int j = 0; j < (int)renderItemMaterials.size(); ++j) {
            //get submesh material properties
            MATERIAL_TYPE matType = renderItemMaterials[j]->GetMatType();
            UINT32 texturesNum = GRAPHICS_CORE::g_materialManager.GetMaterialTypeDescriptorNum(matType);
            
            //initialize the texture num 
            UINT texturesDestNum = texturesNum;
            UINT* srcNums = new UINT[texturesNum];
            for (int i = 0; i < texturesNum; ++i)
                srcNums[i] = 1;

            //allocate the memory descriptor handle
            DescriptorHandle texturesHandle = GRAPHICS_CORE::g_texturesDescriptorHeap.Alloc(texturesNum);
            uint32_t texturesSRVOffset = GRAPHICS_CORE::g_texturesDescriptorHeap.GetOffset(texturesHandle);

            DescriptorHandle samplersHandle = GRAPHICS_CORE::g_samplersDescriptorHeap.Alloc(texturesNum);
            uint32_t samplersSRVOffset = GRAPHICS_CORE::g_samplersDescriptorHeap.GetOffset(samplersHandle);

            std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> textures = renderItemMaterials[j]->GetTextureSRVArray();
            GRAPHICS_CORE::g_device->CopyDescriptors(1, &texturesHandle, &texturesDestNum, texturesDestNum, 
                textures.data(), srcNums, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> samplers = renderItemMaterials[j]->GetSamplerSRVArray();
            GRAPHICS_CORE::g_device->CopyDescriptors(1, &samplersHandle, &texturesDestNum, texturesDestNum,
                samplers.data(), srcNums, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

            //binding the srv information with meshes
            m_renderItems[i].GetTextureSRVOffset().push_back(texturesSRVOffset);
            m_renderItems[i].GetSamplersSRVOffset().push_back(samplersSRVOffset);
        }
    }
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
    (*m_rootSignature)[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_ALL);
    (*m_rootSignature)[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 5, D3D12_SHADER_VISIBILITY_ALL);
    (*m_rootSignature)[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 5, D3D12_SHADER_VISIBILITY_ALL);
    m_rootSignature->Finalize(L"", rootSignatureFlag);
}

void RenderScene::InitializeLights() {
    DirectX::XMFLOAT3 direction = { 1000.0f, 1000.0f, 1000.0f };
    DirectX::XMFLOAT3 color = { 10.0f, 10.0f, 10.0f };
    m_dirLight.InitializeLight(direction, color);
}

void RenderScene::InitializePSO() {
    //Load Shader
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"pbr_vertex.cso", &vertexShaderBlob));
    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"pbr_pixel.cso", &pixelShaderBlob));

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