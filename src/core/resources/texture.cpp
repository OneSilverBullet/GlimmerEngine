#include "texture.h"
#include "graphicscore.h"
#include "context.h"

void Texture::Create2D(size_t rowPitchBytes, size_t width, size_t height, DXGI_FORMAT format, const void* initData)
{
    Destroy();

    m_usageState = D3D12_RESOURCE_STATE_COPY_DEST;

    m_Width = (uint32_t)width;
    m_Height = (uint32_t)height;
    m_Depth = (uint32_t)1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = m_Width;
    desc.Height = m_Height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    //initialize the gpu resource
    GRAPHICS_CORE::g_device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        m_usageState, nullptr, IID_PPV_ARGS(&m_resource));

    m_resource->SetName(L"texture");

    //assemble the texture data in cpu part
    D3D12_SUBRESOURCE_DATA texResource;
    texResource.pData = initData;
    texResource.RowPitch = rowPitchBytes;
    texResource.SlicePitch = rowPitchBytes * height;

    //update the texture data from cpu to gpu resource
    GlobalContext::InitializeTexture(*this, 1, &texResource);

    //generate the srv
    if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
        // texture resource will be stored in whole system
        // so the descriptor is stored in gloabl descriptor heaps
        m_hCpuDescriptorHandle = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    GRAPHICS_CORE::g_device->CreateShaderResourceView(m_resource, nullptr, m_hCpuDescriptorHandle);
}

void Texture::CreateCube(size_t rowPitchBytes, size_t width, size_t height, DXGI_FORMAT format, const void* initData)
{
    Destroy();

    m_usageState = D3D12_RESOURCE_STATE_COPY_DEST;

    m_Width = (uint32_t)width;
    m_Height = (uint32_t)height;
    m_Depth = (uint32_t)6;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = m_Width;
    desc.Height = m_Height;
    desc.DepthOrArraySize = 6;
    desc.MipLevels = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    //initialize the gpu resource
    GRAPHICS_CORE::g_device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &desc,
        m_usageState, nullptr, IID_PPV_ARGS(&m_resource));

    m_resource->SetName(L"texture_cube");

    //assemble the texture data in cpu part
    D3D12_SUBRESOURCE_DATA texResource;
    texResource.pData = initData;
    texResource.RowPitch = rowPitchBytes;
    texResource.SlicePitch = rowPitchBytes * height;

    //update the texture data from cpu to gpu resource
    GlobalContext::InitializeTexture(*this, 1, &texResource);

    //generate the srv
    if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
        // texture resource will be stored in whole system
        // so the descriptor is stored in gloabl descriptor heaps
        m_hCpuDescriptorHandle = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    //create specific srv desc for cube texture 
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MipLevels = 1;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    GRAPHICS_CORE::g_device->CreateShaderResourceView(m_resource, &srvDesc, m_hCpuDescriptorHandle);
}

void Texture::CreateTGAFromMemory(const void* memBuffer, size_t fillSize, bool sRBG)
{
}

bool Texture::CreateDDSFromMemory(const void* memBuffer, size_t fillSize, bool sRGB)
{
    return false;
}

void Texture::CreatePIXImageFromMemory(const void* memBuffer, size_t fileSize)
{
}
