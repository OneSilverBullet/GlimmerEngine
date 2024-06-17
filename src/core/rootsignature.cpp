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

void RootSignature::InitSamplerDesc(UINT registerSlot,
	const D3D12_SAMPLER_DESC& nonStaticSamplerDesc,
	D3D12_SHADER_VISIBILITY visibility) {
	//D3D12_SAMPLER_DESC is suitable for modifying sampler multipyly
	//D3D12_STATIC_SAMPLER_DESC is more efficient cause it is static sampler
	D3D12_STATIC_SAMPLER_DESC& staticSamplerDesc = m_samplers[m_samplersNum++];
	staticSamplerDesc.Filter = nonStaticSamplerDesc.Filter;
	staticSamplerDesc.AddressU = nonStaticSamplerDesc.AddressU;
	staticSamplerDesc.AddressV = nonStaticSamplerDesc.AddressV;
	staticSamplerDesc.AddressW = nonStaticSamplerDesc.AddressW;
	staticSamplerDesc.MipLODBias = nonStaticSamplerDesc.MipLODBias;
	staticSamplerDesc.MaxAnisotropy = nonStaticSamplerDesc.MaxAnisotropy;
	staticSamplerDesc.ComparisonFunc = nonStaticSamplerDesc.ComparisonFunc;
	staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	staticSamplerDesc.MinLOD = nonStaticSamplerDesc.MinLOD;
	staticSamplerDesc.MaxLOD = nonStaticSamplerDesc.MaxLOD;
	staticSamplerDesc.ShaderRegister = registerSlot;
	staticSamplerDesc.RegisterSpace = 0;
	staticSamplerDesc.ShaderVisibility = visibility;

	if (nonStaticSamplerDesc.BorderColor[3] == 1.0f)
	{
		if (nonStaticSamplerDesc.BorderColor[0] == 1.0f)
			staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		else
			staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	}
	else
		staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
}

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
	//TODO: hash function 

	ComPtr<ID3DBlob> pOutBlob, pErrorBlob;

	D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		pOutBlob.GetAddressOf(), pErrorBlob.GetAddressOf());

	ThrowIfFailed(GRAPHICS_CORE::g_device->CreateRootSignature(0, pOutBlob->GetBufferPointer(),
		pOutBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

	m_rootSignature->SetName(name.c_str());

	m_finalized = true;
}