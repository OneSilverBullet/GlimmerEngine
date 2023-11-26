#pragma once
#include "gpuresource.h"

class GPUBuffer : public GPUResource
{
public:
	virtual ~GPUBuffer() { Destroy(); }

	void Create(const std::wstring& name, uint32_t elements, uint32_t elementSize, const void* data = nullptr);


protected:
	GPUBuffer() :m_elementCount(0), m_elementSize(0), m_bufferSize(0) {
		m_resourceFlag = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		m_uav.ptr = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
		m_srv.ptr = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
	}
	D3D12_RESOURCE_DESC DescribeBuffer();
	virtual void CreateDerivedViews() = 0;


protected:
	D3D12_CPU_DESCRIPTOR_HANDLE m_uav;
	D3D12_CPU_DESCRIPTOR_HANDLE m_srv;

	size_t m_bufferSize;
	uint32_t m_elementCount;
	uint32_t m_elementSize;
	D3D12_RESOURCE_FLAGS m_resourceFlag;
};