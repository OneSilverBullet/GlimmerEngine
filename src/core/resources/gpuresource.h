#pragma once
#include "headers.h"

/*
* GPUResource: the basic class for buffer
*/
class GPUResource
{
public:
	GPUResource() : m_resource(nullptr), m_gpuAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
		m_usageState(D3D12_RESOURCE_STATE_COMMON),
		m_transmissionState((D3D12_RESOURCE_STATES)-1) {}
	
	GPUResource(ID3D12Resource* instance, D3D12_RESOURCE_STATES usage) :
		m_resource(instance), m_gpuAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
		m_usageState(usage), m_transmissionState((D3D12_RESOURCE_STATES)-1) {}

	~GPUResource() { Destroy(); }

	virtual void Destroy() {
		if (m_resource != nullptr) {
			m_resource->Release();
			m_resource = nullptr;
		}
		m_gpuAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
	}

	ID3D12Resource* operator->() { return m_resource; }
	const ID3D12Resource* operator->() const { return m_resource; }

	ID3D12Resource* GetResource() { return m_resource; }
	const ID3D12Resource* GetResource() const { return m_resource; }

	ID3D12Resource** GetAddressOf() { return &m_resource; }
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() { return m_gpuAddress; }
	D3D12_RESOURCE_STATES GetUsageState() { return m_usageState; }
	void SetUsageState(D3D12_RESOURCE_STATES v) { m_usageState = v; }

protected:
	ID3D12Resource* m_resource;
	D3D12_RESOURCE_STATES m_usageState;
	D3D12_RESOURCE_STATES m_transmissionState;
	D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddress;
};


