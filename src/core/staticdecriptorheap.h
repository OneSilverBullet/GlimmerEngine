#pragma once

#include <mutex>
#include <string>
#include "headers.h"

// the descriptor handle
class DescriptorHandle
{
public:
	DescriptorHandle() {
		m_cpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
		m_gpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) 
		: m_cpuHandle(cpuHandle), m_gpuHandle(gpuHandle)
	{
	}

	DescriptorHandle operator+(INT offsetScaledByDescriptorSize) const {
		DescriptorHandle ret = *this;
		ret += offsetScaledByDescriptorSize;
		return ret;
	}

	void operator+=(INT offset)
	{
		if (!IsNull()) m_cpuHandle.ptr += offset;
		if (IsShaderVisible()) m_gpuHandle.ptr += offset;
	}

	//implided convert to descriptor handle
	operator D3D12_CPU_DESCRIPTOR_HANDLE()const { return m_cpuHandle; }
	operator D3D12_GPU_DESCRIPTOR_HANDLE()const { return m_gpuHandle; }

	size_t GetCPUPtr() const { return m_cpuHandle.ptr; }
	uint64_t GetGPUPtr() const { return m_gpuHandle.ptr; }
	bool IsNull() const { return m_cpuHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_NULL; }
	bool IsShaderVisible() const { return m_gpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }

private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle;
};


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


