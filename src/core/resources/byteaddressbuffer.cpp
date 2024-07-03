#pragma once
#include "uploadbuffer.h"
#include "graphicscore.h"
#include "byteaddressbuffer.h"


void ByteAddressBuffer::CreateDerivedViews(void)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Buffer.NumElements = (UINT)m_bufferSize / 4;
    SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

    if (m_srv.ptr == D3D12_GPU_VIRTUAL_ADDRESS_NULL)
        m_srv = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    GRAPHICS_CORE::g_device->CreateShaderResourceView(m_resource, &SRVDesc, m_srv);

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    UAVDesc.Buffer.NumElements = (UINT)m_bufferSize / 4;
    UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

    if (m_uav.ptr == D3D12_GPU_VIRTUAL_ADDRESS_NULL)
        m_uav = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    GRAPHICS_CORE::g_device->CreateUnorderedAccessView(m_resource, nullptr, &UAVDesc, m_uav);
}
