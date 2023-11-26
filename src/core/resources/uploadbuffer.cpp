#pragma once
#include "uploadbuffer.h"
#include "graphicscore.h"

void UploadBuffer::Create(const std::wstring& name, size_t bufferSize) {
	Destroy();

	m_bufferSize = bufferSize;

    //Generate Heap Property for Upload Buffer
	D3D12_HEAP_PROPERTIES heapProp;
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 0;
	heapProp.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC desc = {};
    desc.Alignment = 0;
    desc.DepthOrArraySize = 1;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.Height = 1;
    desc.Width = m_bufferSize;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    ThrowIfFailed(GRAPHICS_CORE::g_device->CreateCommittedResource(
        &heapProp, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, 
        nullptr, IID_PPV_ARGS(&m_resource)
    ));

    m_gpuAddress = m_resource->GetGPUVirtualAddress();
    m_resource->SetName(name.c_str());
}

void* UploadBuffer::Map() {
    void* memory = nullptr;
    m_resource->Map(0, &CD3DX12_RANGE(0, m_bufferSize), &memory);
    return memory;
}

void UploadBuffer::Unmap(size_t begin, size_t end) {
    m_resource->Unmap(0, &CD3DX12_RANGE(begin, std::min(end, m_bufferSize)));
}
