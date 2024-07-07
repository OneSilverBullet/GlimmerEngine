#pragma once
#include "gpuresource.h"


class Texture : public GPUResource
{
public:
	Texture() : m_Width(0), m_Height(0), m_Depth(0) { m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }
	Texture(D3D12_CPU_DESCRIPTOR_HANDLE handle) : m_Width(0), m_Height(0), m_Depth(0), m_hCpuDescriptorHandle(handle){}

	//create a textures
	void Create2D(size_t rowPitchBytes, size_t width, size_t height, DXGI_FORMAT format, const void* initData);
	void CreateCube(size_t rowPitchBytes, size_t width, size_t height, DXGI_FORMAT format, const void* initData, D3D12_RESOURCE_STATES usage = D3D12_RESOURCE_STATE_GENERIC_READ);


	bool CreateDDSFromMemory(const void* memBuffer, size_t fillSize, bool sRGB);


	virtual void Destroy() override
	{
		GPUResource::Destroy();
		m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	uint32_t GetWidth() { return m_Width; }
	uint32_t GetHeight() { return m_Height; }
	uint32_t GetDepth() { return m_Depth; }
	const D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() { return m_hCpuDescriptorHandle; }

protected:
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_Depth;
	
	D3D12_CPU_DESCRIPTOR_HANDLE m_hCpuDescriptorHandle;

};

enum DefaultTextureType {
	Magenta2D = 0,
	BlackOpaque2D = 1,
	BlackTransparent2D = 2,
	WhiteOpaque2D = 3,
	WhiteTransparent2D = 4,
	DefaultNormalMap2D = 5,
	BlackCubeMap = 6,

	DefaultTextureNum = 7,
};
