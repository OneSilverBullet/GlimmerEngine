#include "readbackbuffer.h"
#include "graphicscore.h"


void ReadbackBuffer::Create(const std::wstring& name, 
	uint32_t numElements, uint32_t elementSize) {
	Destroy();

	m_elementCount = numElements;
	m_elementSize = m_elementSize;
	m_bufferSize = m_elementCount * m_elementSize;
	m_usageState = D3D12_RESOURCE_STATE_COPY_DEST;

	D3D12_HEAP_PROPERTIES heapProps;
	heapProps.Type = D3D12_HEAP_TYPE_READBACK;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	//readback buffer must be 1-dimention buffer
	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = m_bufferSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	assert(GRAPHICS_CORE::g_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_resource)));

	m_gpuAddress = m_resource->GetGPUVirtualAddress();

}

void* ReadbackBuffer::Map() {

	void* memory;
	m_resource->Map(0, &CD3DX12_RANGE(0, m_bufferSize), &memory);
	return memory;
}

void ReadbackBuffer::Unmap() {
	m_resource->Unmap(0, &CD3DX12_RANGE(0, 0));
}
