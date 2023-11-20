#pragma once

#include "pixelbuffer.h"

//Depth Buffer is used for depth buffer and stencil buffer
class DepthBuffer : public PixelBuffer
{
public:
	DepthBuffer(float clearDepth = 1.0f, uint8_t clearStencil = 0) :
		m_clearDepth(clearDepth), m_clearStencil(clearStencil){
		m_hdsv[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		m_hdsv[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		m_hdsv[2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		m_hdsv[3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		m_hDepthSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		m_hStencilSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	void Create(const std::wstring& name, uint32_t width, uint32_t height,
		DXGI_FORMAT format);

	void Create(const std::wstring& name, uint32_t width, uint32_t height, D3D12_RESOURCE_STATES initialState,
		DXGI_FORMAT format);

	void Create(const std::wstring& name, uint32_t width, uint32_t height, uint32_t samplesNum,
		DXGI_FORMAT format);

	const D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() { return m_hdsv[0]; }
	const D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_depthonly() { return m_hdsv[1]; }
	const D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_stencilonly() { return m_hdsv[2]; }
	const D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_readonly() { return m_hdsv[3]; }
	const D3D12_CPU_DESCRIPTOR_HANDLE GetDepthSRV() { return m_hDepthSRV; }
	const D3D12_CPU_DESCRIPTOR_HANDLE GetStencilSRV() { return m_hStencilSRV; }
	float GetClearValue() { return m_clearDepth; }
	uint8_t GetStencilValue() { return m_clearStencil; }

protected:
	void CreateDerivedViews(ID3D12Device* device, DXGI_FORMAT format);

	float m_clearDepth;
	uint8_t m_clearStencil;

	D3D12_CPU_DESCRIPTOR_HANDLE m_hdsv[4];
	D3D12_CPU_DESCRIPTOR_HANDLE m_hDepthSRV;
	D3D12_CPU_DESCRIPTOR_HANDLE m_hStencilSRV;



};

