
#include "staticdecriptorheap.h"
#include "graphicscore.h"



void StaticDescriptorHeap::Initialize(const std::wstring& heapName, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t maxDescriptorsNum) {
	m_heapDesc.Type = type;
	m_heapDesc.NumDescriptors = maxDescriptorsNum;
	m_heapDesc.NodeMask = 0;
	m_heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(GRAPHICS_CORE::g_device->CreateDescriptorHeap(&m_heapDesc, IID_PPV_ARGS(&m_descriptorHeap)));
	m_descriptorHeap->SetName(heapName.c_str());

	//initialize the basic parameter
	m_descriptorSize = GRAPHICS_CORE::g_device->GetDescriptorHandleIncrementSize(type);
	m_numFreeDescriptors = maxDescriptorsNum;
	m_firstHandle = DescriptorHandle(
		m_descriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		m_descriptorHeap->GetGPUDescriptorHandleForHeapStart()
	);
	m_nextFreeHandle = m_firstHandle;
}

void StaticDescriptorHeap::Release() {
	if (m_descriptorHeap != nullptr) {
		m_descriptorHeap->Release();
		m_descriptorHeap = nullptr;
	}
}

DescriptorHandle StaticDescriptorHeap::Alloc(uint32_t count) {
	assert(HasAvailableSpace(count));
	DescriptorHandle ret = m_nextFreeHandle;
	m_nextFreeHandle += count * m_descriptorSize;
	m_numFreeDescriptors -= count;
	return ret;
}


bool StaticDescriptorHeap::ValidateHandle(const DescriptorHandle& dhandle) const {
	if (dhandle.GetCPUPtr() < m_firstHandle.GetCPUPtr() || 
		dhandle.GetCPUPtr() >= m_firstHandle.GetCPUPtr() + m_descriptorSize * m_heapDesc.NumDescriptors)
		return false;

	if (dhandle.GetCPUPtr() - m_firstHandle.GetCPUPtr() != dhandle.GetGPUPtr() - m_firstHandle.GetGPUPtr())
		return false;

	return true;
}