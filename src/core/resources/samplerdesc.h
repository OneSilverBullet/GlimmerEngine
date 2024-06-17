#pragma once
#include "d3d12.h"

class SamplerDesc : public D3D12_SAMPLER_DESC
{
public:
	SamplerDesc();

	void SetAddressMode(D3D12_TEXTURE_ADDRESS_MODE addressMode);

	D3D12_CPU_DESCRIPTOR_HANDLE CreateSamplerDescHandle();
	void CreateSamplerDescHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle);
};