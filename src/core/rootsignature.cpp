#include "rootsignature.h"
#include "graphicscore.h"


/*
* RootSignatureManager
* There are a great number of root signatures in graphics applications.
* For the efficiency, we reuse the root signature which has the same descriptors layout 
* Each root signature should be stored in this singleton.
*/
void RootSignatureManager::Insert(uint32_t hashValue, ID3D12RootSignature* rootSignatureCompiled) {
	assert(rootSignatureCompiled != nullptr);
	m_storage[hashValue].Attach(rootSignatureCompiled);
}

ID3D12RootSignature* RootSignatureManager::Get(uint32_t hashValue) {
	if (m_storage.find(hashValue) != m_storage.end())
		return m_storage[hashValue].Get();
	return nullptr;
}

void RootSignatureManager::Release() {
	m_storage.clear();
}

/*
* RootSignature
* Root Signature is a description of the input data types of the shader
*/
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