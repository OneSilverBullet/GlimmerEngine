#pragma once
#include "samplerdesc.h"
#include "graphicscore.h"

SamplerDesc::SamplerDesc()
{
	Filter = D3D12_FILTER_ANISOTROPIC;
	AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	MipLODBias = 0.0f;
	MaxAnisotropy = 16;
	ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	BorderColor[0] = 1.0f;
	BorderColor[1] = 1.0f;
	BorderColor[2] = 1.0f;
	BorderColor[3] = 1.0f;
	MinLOD = 0.0f;
	MaxLOD = D3D12_FLOAT32_MAX;
}

void SamplerDesc::SetAddressMode(D3D12_TEXTURE_ADDRESS_MODE addressMode) {
	AddressU = addressMode;
	AddressV = addressMode;
	AddressW = addressMode;
}

D3D12_CPU_DESCRIPTOR_HANDLE SamplerDesc::CreateSamplerDescHandle()
{
	D3D12_CPU_DESCRIPTOR_HANDLE allocatedHandle = GRAPHICS_CORE::AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	GRAPHICS_CORE::g_device->CreateSampler(this, allocatedHandle);
	return allocatedHandle;
}

void SamplerDesc::CreateSamplerDescHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	GRAPHICS_CORE::g_device->CreateSampler(this, handle);
}
