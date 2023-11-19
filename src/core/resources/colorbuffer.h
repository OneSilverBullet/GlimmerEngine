#pragma once

#include "pixelbuffer.h"
#include "types/color.h"

class ColorBuffer : public PixelBuffer
{
public:
	ColorBuffer(Color clearColor = Color(0.0f, 0.0f, 0.0f, 0.0f));

	void CreateFromSwapChain(const std::wstring& name, ID3D12Resource* baseResource);

	void CreateBuffer(const std::wstring& name, uint32_t width, uint32_t height, uint32_t mips,
		DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	void CreateBufferArray(const std::wstring& name, uint32_t width, uint32_t height, uint32_t arrayCount,
		DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return m_srvHandle; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTV() const { return m_rtvHandle; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV() const { return m_uavHandle[0]; }

	void SetClearColor(Color clearColor) { m_clearColor = clearColor; }

	void SetMsaaMode(uint32_t numColorSamples, uint32_t numCoverageSamples) {
		assert(numCoverageSamples >= numColorSamples);
		m_fragmentCount = numColorSamples;
		m_sampleCount = numCoverageSamples;
	}

	Color GetClearColor() const { return m_clearColor; }

	//void GenerateMipMaps(CommandContext& Context);

protected:
	D3D12_RESOURCE_FLAGS CombineResourceFlag() const {
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
		if (flags == D3D12_RESOURCE_FLAG_NONE && m_fragmentCount == 1)
			flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | flags;
	}

	static inline uint32_t ComputeNumMips(uint32_t width, uint32_t height) {
		uint32_t highbit;
		_BitScanReverse((unsigned long*)&highbit, width | height);
		return highbit + 1;
	}
	
	void CreateDerivedViews(ID3D12Device* device, DXGI_FORMAT format,
		uint32_t arraySize, uint32_t numMips = 1);

	D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_uavHandle[12];
	uint32_t m_mips;
	uint32_t m_fragmentCount;
	uint32_t m_sampleCount;
	Color m_clearColor;
};


