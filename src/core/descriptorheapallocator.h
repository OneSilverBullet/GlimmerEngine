#pragma once

#include <mutex>
#include <vector>
#include <string>
#include <d3d12.h>
#include <wrl/client.h>

/*
* Descriptor Allocator is responsible for allocate the descriptor 
* Static Descriptor Heap: for the descriptor of textures, samples
* Dynamic Descriptor Heap: for the descriptors that only use in one frame
*/
class DescriptorAllocator
{
public:
	DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type);

	D3D12_CPU_DESCRIPTOR_HANDLE Allocator(uint32_t count);

	static void DestroyAll();

protected:
	static const uint32_t g_numDescriptorPerHeap = 256; //the number of the descriptors in each heap
	static std::mutex g_allocatorMutex; //resource critical section
	static std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> g_descriptorPool; //descriptor heap pool
	static ID3D12DescriptorHeap* RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);

	D3D12_DESCRIPTOR_HEAP_TYPE m_type; //heap type
	ID3D12DescriptorHeap* m_currentHeap; //current heap
	D3D12_CPU_DESCRIPTOR_HANDLE m_currentHandle; //current heap head ptr
	uint32_t m_descriptorSize; //the size of each descriptor
	uint32_t m_remainingFreeHandles; // the remain space
};
