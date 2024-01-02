#pragma once

#include <mutex>
#include <string>
#include "headers.h"
#include "descriptortypes.h"

//StaticDescriptorHeap: for static descriptor 
class StaticDescriptorHeap
{
public:
	StaticDescriptorHeap() : m_descriptorHeap(nullptr){}
	~StaticDescriptorHeap() { Release(); }

	void Initialize(const std::wstring& heapName, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t maxDescriptorsNum);
	void Release();

	bool HasAvailableSpace(uint32_t count) const { return count <= m_numFreeDescriptors; }
	DescriptorHandle Alloc(uint32_t count = 1);

	DescriptorHandle operator[](uint32_t arrayIndex) { return m_firstHandle + arrayIndex * m_descriptorSize; }

	uint32_t GetOffset(const DescriptorHandle& handle) {
		return (uint32_t)(handle.GetCPUPtr() - m_firstHandle.GetCPUPtr()) / m_descriptorSize;
	}
	//check current handle is legal in this descriptor heap
	bool ValidateHandle(const DescriptorHandle& dhandle) const;
	ID3D12DescriptorHeap* GetDescriptorHeap() { return m_descriptorHeap; }
	uint32_t GetDescriptorSize()const { return m_descriptorSize; }

private:
	ID3D12DescriptorHeap* m_descriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC m_heapDesc;
	uint32_t m_descriptorSize;
	uint32_t m_numFreeDescriptors;
	DescriptorHandle m_firstHandle;
	DescriptorHandle m_nextFreeHandle;
};


