#include "descriptorheapallocator.h"
#include "headers.h"
#include "graphicscore.h"

std::mutex DescriptorAllocator::g_allocatorMutex; //resource critical section
std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> DescriptorAllocator::g_descriptorPool; //descriptor heap pool

DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type):
	m_type(type), m_currentHeap(nullptr), m_descriptorSize(0), m_remainingFreeHandles(0)
{
	m_currentHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::Allocator(uint32_t count) {
	//The first initialize of the current heap, there is only one heap for per descriptor type
	if (m_currentHeap != nullptr || m_remainingFreeHandles < count) {
		m_currentHeap = RequestNewHeap(m_type);
		m_currentHandle = m_currentHeap->GetCPUDescriptorHandleForHeapStart();
		m_remainingFreeHandles = DescriptorAllocator::g_numDescriptorPerHeap;
		if (m_descriptorSize == 0)
			m_descriptorSize = GRAPHICS_CORE::g_device->GetDescriptorHandleIncrementSize(m_type);
	}

	//return a ptr on the heap
	D3D12_CPU_DESCRIPTOR_HANDLE ret = m_currentHandle;
	m_currentHandle.ptr += count * m_descriptorSize;
	m_remainingFreeHandles -= count;
	return ret;
}

void DescriptorAllocator::DestroyAll() {
	DescriptorAllocator::g_descriptorPool.clear(); //clear the descriptor pool
}

ID3D12DescriptorHeap* DescriptorAllocator::RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) {
	std::lock_guard<std::mutex> resourceLock(DescriptorAllocator::g_allocatorMutex);

	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.Type = type;
	desc.NumDescriptors = DescriptorAllocator::g_numDescriptorPerHeap;
	desc.NodeMask = 0;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> newHeap;
	ThrowIfFailed(GRAPHICS_CORE::g_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&newHeap)));
	DescriptorAllocator::g_descriptorPool.push_back(newHeap);
	return newHeap.Get();
}

