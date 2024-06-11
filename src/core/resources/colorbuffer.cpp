#include "colorbuffer.h"
#include "graphicscore.h"
#include <iostream>

ColorBuffer::ColorBuffer(Color clearColor)
	: m_clearColor(clearColor), m_mips(0), m_fragmentCount(1), m_sampleCount(1)
{
	m_rtvHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	m_srvHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	for (int i = 0; i < _countof(m_uavHandle); ++i)
		m_uavHandle[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;


	std::cout << "color buffer init" << std::endl;
}

void ColorBuffer::CreateFromSwapChain(const std::wstring& name, ID3D12Resource* baseResource) {
	AssociateWithResource(GRAPHICS_CORE::g_device, name, baseResource, D3D12_RESOURCE_STATE_PRESENT);
	m_rtvHandle = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	GRAPHICS_CORE::g_device->CreateRenderTargetView(m_resource, nullptr, m_rtvHandle);
}

void ColorBuffer::CreateBuffer(const std::wstring& name, uint32_t width, uint32_t height, uint32_t mips,
	DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr) {
	mips = (mips == 0 ? ComputeNumMips(width, height) : mips);
	D3D12_RESOURCE_FLAGS flags = CombineResourceFlag();
	D3D12_RESOURCE_DESC resourceDesc = DescribeTex2D(width, height, 1, mips, format, flags);

	resourceDesc.SampleDesc.Count = m_fragmentCount;
	resourceDesc.SampleDesc.Quality = 0;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.Color[0] = m_clearColor.R();
	clearValue.Color[1] = m_clearColor.G();
	clearValue.Color[2] = m_clearColor.B();
	clearValue.Color[3] = m_clearColor.A();

	CreateTextureResource(GRAPHICS_CORE::g_device, name, resourceDesc, clearValue, vidMemPtr);
	CreateDerivedViews(GRAPHICS_CORE::g_device, format, 1, mips);
}

void ColorBuffer::CreateBufferArray(const std::wstring& name, uint32_t width, uint32_t height, uint32_t arrayCount,
	DXGI_FORMAT format, D3D12_GPU_VIRTUAL_ADDRESS vidMemPtr) {
	
	D3D12_RESOURCE_FLAGS flags = CombineResourceFlag();
	D3D12_RESOURCE_DESC resourceDesc = DescribeTex2D(width, height, arrayCount, 1, format, flags);

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.Color[0] = m_clearColor.R();
	clearValue.Color[1] = m_clearColor.G();
	clearValue.Color[2] = m_clearColor.B();
	clearValue.Color[3] = m_clearColor.A();

	CreateTextureResource(GRAPHICS_CORE::g_device, name, resourceDesc, clearValue, vidMemPtr);
	CreateDerivedViews(GRAPHICS_CORE::g_device, format, arrayCount, 1);
}

void ColorBuffer::CreateDerivedViews(ID3D12Device* device, DXGI_FORMAT format,
	uint32_t arraySize, uint32_t numMips) {
	assert(arraySize == 1 || numMips == 1, "We dont support auto-mips on texture arrays");

	m_mips = numMips - 1;

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	rtvDesc.Format = format;
	uavDesc.Format = GetUAVFormat(format);
	srvDesc.Format = format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;


	if (arraySize > 1) //for texture array
	{
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = 0;
		rtvDesc.Texture2DArray.ArraySize = (UINT)arraySize;

		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.MipSlice = 0;
		uavDesc.Texture2DArray.FirstArraySlice = 0;
		uavDesc.Texture2DArray.ArraySize = (UINT)arraySize;

		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MipLevels = numMips;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = (UINT)arraySize;
	}
	else if (m_fragmentCount > 1) //for msaa
	{
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	else //for a single texture
	{
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;

		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = numMips;
		srvDesc.Texture2D.MostDetailedMip = 0;
	}

	//Create RTV and SRV
	if (m_srvHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
	{
		m_rtvHandle = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_srvHandle = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	device->CreateRenderTargetView(m_resource, &rtvDesc, m_rtvHandle);
	device->CreateShaderResourceView(m_resource, &srvDesc, m_srvHandle);

	//if msaa is enabled, there is no need to create uav
	if (m_fragmentCount > 1)
		return;

	// Create the UAVs for each mip level (RWTexture2D) 
	for (uint32_t i = 0; i < numMips; ++i)
	{
		if (m_uavHandle[i].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			m_uavHandle[i] = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		GRAPHICS_CORE::g_device->CreateUnorderedAccessView(m_resource, nullptr, &uavDesc, m_uavHandle[i]);

		uavDesc.Texture2D.MipSlice++;
	}
}

