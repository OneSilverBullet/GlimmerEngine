#pragma once
#include "gpuresource.h"

class UploadBuffer;

class GPUBuffer : public GPUResource
{
public:
	virtual ~GPUBuffer() { Destroy(); }

	void Create(const std::wstring& name, uint32_t elements, uint32_t elementSize, const void* data = nullptr);
	
	void Create(const std::wstring& name, uint32_t elements, uint32_t elementSize,
		const UploadBuffer& srcData, uint32_t srcOffset = 0);

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV()const { return m_uav; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV()const { return m_srv; }

	D3D12_GPU_VIRTUAL_ADDRESS RootConstantBufferView()const { return m_gpuAddress; }

	D3D12_CPU_DESCRIPTOR_HANDLE CreateConstantBufferView(uint32_t offset, uint32_t size) const;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t offset, uint32_t size, uint32_t stride) const;
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t baseVertexIndex = 0);
	D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t offset, uint32_t size, bool b32Bit = false) const;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t startIndex = 0) const;

	size_t GetBufferSize()const { return m_bufferSize; }
	size_t GetElementCount() const { return m_elementCount; }
	size_t GetElementSize() const { return m_elementSize; }

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