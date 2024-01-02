#pragma once

#include <mutex>
#include <vector>
#include <string>
#include <d3d12.h>
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

//describe the detail of a descriptor table entry
struct DescriptorTableEntry
{
	DescriptorTableEntry() : assignedHandlesBitMap(0), tableSize(0), tableStart(nullptr){}
	uint32_t assignedHandlesBitMap;
	D3D12_CPU_DESCRIPTOR_HANDLE* tableStart;
	uint32_t tableSize;
};








