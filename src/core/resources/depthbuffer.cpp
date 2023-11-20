#include "depthbuffer.h"
#include "graphicscore.h"

void DepthBuffer::Create(const std::wstring& name, uint32_t width, uint32_t height,
	DXGI_FORMAT format) {
	Create(name, width, height, 1, format);
}


void DepthBuffer::Create(const std::wstring& name, uint32_t width, uint32_t height, D3D12_RESOURCE_STATES initialState,
	DXGI_FORMAT format) {
	D3D12_RESOURCE_DESC resourceDesc = DescribeTex2D(width, height, 1, 1, format,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	resourceDesc.SampleDesc.Count = 1;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.DepthStencil.Depth = m_clearDepth;
	clearValue.DepthStencil.Stencil = m_clearStencil;
	CreateTextureResource(GRAPHICS_CORE::g_device, name, resourceDesc, clearValue, initialState);
	CreateDerivedViews(GRAPHICS_CORE::g_device, format);
}

void DepthBuffer::Create(const std::wstring& name, uint32_t width, uint32_t height, uint32_t samplesNum,
	DXGI_FORMAT format) {
	D3D12_RESOURCE_DESC resourceDesc = DescribeTex2D(width, height, 1, 1, format, 
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	resourceDesc.SampleDesc.Count = samplesNum;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.DepthStencil.Depth = m_clearDepth;
	clearValue.DepthStencil.Stencil = m_clearStencil;
	CreateTextureResource(GRAPHICS_CORE::g_device, name, resourceDesc, clearValue);
	CreateDerivedViews(GRAPHICS_CORE::g_device, format);
}

void DepthBuffer::CreateDerivedViews(ID3D12Device* device, DXGI_FORMAT format) {
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = GetDSVFormat(format);
	
	//Build texture
	if (m_resource->GetDesc().SampleDesc.Count == 1) {
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
	}
	else {
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	}

	//Construct A Depth Stencil View
	if (m_hdsv[0].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
	{
		m_hdsv[0] = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_hdsv[1] = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	device->CreateDepthStencilView(m_resource, &dsvDesc, m_hdsv[0]);
	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	device->CreateDepthStencilView(m_resource, &dsvDesc, m_hdsv[1]);

	//if availiable for stencil, create stencil view
	DXGI_FORMAT stencilReadFormat = GetStencilFormat(format);
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN) {
		if (m_hdsv[2].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
			m_hdsv[2] = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			m_hdsv[3] = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		}
		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		device->CreateDepthStencilView(m_resource, &dsvDesc, m_hdsv[2]);
		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		device->CreateDepthStencilView(m_resource, &dsvDesc, m_hdsv[3]);
	}
	else {
		m_hdsv[2] = m_hdsv[0];
		m_hdsv[3] = m_hdsv[1];
	}

	//Create Depth Shader Resource View
	if (m_hDepthSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_hDepthSRV = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = GetDepthFormat(format);
	if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D)
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
	}
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	device->CreateShaderResourceView(m_resource, &srvDesc, m_hDepthSRV);

	//Create Stencil Shader Resource View
	if(stencilReadFormat != DXGI_FORMAT_UNKNOWN) {
		if (m_hStencilSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			m_hStencilSRV = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		srvDesc.Format = stencilReadFormat;
		device->CreateShaderResourceView(m_resource, &srvDesc, m_hStencilSRV);
	}
}

