#include "rootsignature.h"
#include "graphicscore.h"

void RootSignature::Finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS Flags) {

	if(m_finalized)
		return;

	
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.NumParameters = m_parametersNum;
	rootSignatureDesc.NumStaticSamplers = m_samplersNum;
	rootSignatureDesc.pParameters = (const D3D12_ROOT_PARAMETER*)m_parameters.get();
	rootSignatureDesc.pStaticSamplers = (const D3D12_STATIC_SAMPLER_DESC*)m_samplers.get();
	rootSignatureDesc.Flags = Flags;


	ComPtr<ID3DBlob> pOutBlob, pErrorBlob;

	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		pOutBlob.GetAddressOf(), pErrorBlob.GetAddressOf());

	ThrowIfFailed(GRAPHICS_CORE::g_device->CreateRootSignature(0, pOutBlob->GetBufferPointer(),
		pOutBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

	m_rootSignature->SetName(name.c_str());


	m_finalized = true;
}