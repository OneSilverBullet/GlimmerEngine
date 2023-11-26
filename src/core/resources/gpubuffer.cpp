#pragma once
#include "gpubuffer.h"
#include "graphicscore.h"

void GPUBuffer::Create(const std::wstring& name, uint32_t elements, 
	uint32_t elementSize, const void* data) {

	Destroy();

	m_elementCount = elements;
	m_elementSize = elementSize;
	m_bufferSize = m_elementCount * m_elementSize;
	m_usageState = D3D12_RESOURCE_STATE_COMMON;

	D3D12_RESOURCE_DESC m_resourceDesc = DescribeBuffer();

	D3D12_HEAP_PROPERTIES heapProp;
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.CreationNodeMask = 0;
	heapProp.VisibleNodeMask = 0;

	ThrowIfFailed(GRAPHICS_CORE::g_device->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE, &m_resourceDesc, m_usageState, nullptr, IID_PPV_ARGS(&m_resource)
	));
	m_resource->SetName(name.c_str());


	


	CreateDerivedViews();
}


D3D12_RESOURCE_DESC GPUBuffer::DescribeBuffer() {



}

