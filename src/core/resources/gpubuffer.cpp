#pragma once
#include "gpubuffer.h"
#include "graphicscore.h"

void GPUBuffer::Create(const std::wstring& name, uint32_t elements, 
	uint32_t elementSize, const void* data) {
	//clear previous resource
	Destroy();

	m_elementCount = elements;
	m_elementSize = elementSize;
	m_bufferSize = m_elementCount * m_elementSize;
	m_usageState = D3D12_RESOURCE_STATE_COMMON;

	//describe the default gpu buffer 
	D3D12_RESOURCE_DESC m_resourceDesc = DescribeBuffer();

	//create the heap for current buffer
	D3D12_HEAP_PROPERTIES heapProp;
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.CreationNodeMask = 0;
	heapProp.VisibleNodeMask = 0;

	//create buffer
	ThrowIfFailed(GRAPHICS_CORE::g_device->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE, &m_resourceDesc, m_usageState, nullptr, IID_PPV_ARGS(&m_resource)
	));
	m_resource->SetName(name.c_str());
	m_gpuAddress = m_resource->GetGPUVirtualAddress();

	//TODO: copy the data to current gpu buffer, linear memory system allocate memory to current buffer



	//create related views 
	CreateDerivedViews();
}


D3D12_RESOURCE_DESC GPUBuffer::DescribeBuffer() {
	D3D12_RESOURCE_DESC desc = {};
	desc.Alignment = 0;
	desc.DepthOrArraySize = 1;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Flags = m_resourceFlag;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.Height = 1;
	desc.Width = (UINT64)m_bufferSize;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	return desc;
}


D3D12_CPU_DESCRIPTOR_HANDLE GPUBuffer::CreateConstantBufferView(uint32_t offset, uint32_t size) const {

}

//Initialize gpu buffer as the vertex buffer
D3D12_VERTEX_BUFFER_VIEW GPUBuffer::VertexBufferView(size_t offset, uint32_t size, 
	uint32_t stride) const {
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	vertexBufferView.BufferLocation = m_gpuAddress + offset;
	vertexBufferView.SizeInBytes = size;
	vertexBufferView.StrideInBytes = stride;
	return vertexBufferView;
}

D3D12_VERTEX_BUFFER_VIEW GPUBuffer::VertexBufferView(size_t baseVertexIndex) {
	uint32_t offset = baseVertexIndex * m_elementSize;
	return VertexBufferView(offset, (uint32_t)(m_bufferSize - offset), m_elementSize);
}

//Initialize gpu buffer as the index buffer
D3D12_INDEX_BUFFER_VIEW GPUBuffer::IndexBufferView(size_t offset, uint32_t size, bool b32Bit) const {
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	indexBufferView.BufferLocation = m_gpuAddress + offset;
	indexBufferView.Format = b32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
	indexBufferView.SizeInBytes = size;
	return indexBufferView;
}

D3D12_INDEX_BUFFER_VIEW GPUBuffer::IndexBufferView(size_t startIndex) const {
	size_t offset = startIndex * m_elementSize;
	return IndexBufferView(offset, (uint32_t)(m_bufferSize - offset), m_elementSize == 4);
}


