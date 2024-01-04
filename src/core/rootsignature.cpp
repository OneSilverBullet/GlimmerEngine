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

	m_descriptorTableBitMap = 0;
	m_samplerBitMap = 0;
	
	//extract the parameters information in current root signature
	for (int paramIndex = 0; paramIndex < m_parametersNum; ++paramIndex)
	{
		D3D12_ROOT_PARAMETER parameter = rootSignatureDesc.pParameters[paramIndex];

		if (parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			//check the descriptor table is valid
			assert(parameter.DescriptorTable.pDescriptorRanges != nullptr);

			//current descriptor table stores multiple samplers
			if (parameter.DescriptorTable.pDescriptorRanges->RangeType ==
				D3D12_DESCRIPTOR_RANGE_TYPE::D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
				m_samplerBitMap |= (1 << paramIndex);
			else
				m_descriptorTableBitMap |= (1 << paramIndex);

			//record the descriptors size in current entry
			for (int subDescriptorRangeIndex = 0; subDescriptorRangeIndex < parameter.DescriptorTable.NumDescriptorRanges; ++subDescriptorRangeIndex)
			{
				m_descriptorTableSize[paramIndex] += parameter.DescriptorTable.pDescriptorRanges[subDescriptorRangeIndex].NumDescriptors;
			}
		}
	}

	//TODO: Do not compile the root signature every time; reuse the same parameter layout root signature

	ComPtr<ID3DBlob> pOutBlob, pErrorBlob;

	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		pOutBlob.GetAddressOf(), pErrorBlob.GetAddressOf());

	ThrowIfFailed(GRAPHICS_CORE::g_device->CreateRootSignature(0, pOutBlob->GetBufferPointer(),
		pOutBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

	m_rootSignature->SetName(name.c_str());

	m_finalized = true;
}