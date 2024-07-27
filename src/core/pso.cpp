#include "pso.h"
#include "rootsignature.h"
#include "graphicscore.h"

/*
* GraphicsPSO
*/
GraphicsPSO::GraphicsPSO(const wchar_t* name) :PSO(name) {
	//Parameter initialization
	::ZeroMemory(&m_psoDesc, sizeof(m_psoDesc));
	m_psoDesc.NodeMask = 0;
	m_psoDesc.SampleMask = 0xFFFFFFFFu;
	m_psoDesc.SampleDesc.Count = 1;
	m_psoDesc.InputLayout.NumElements = 0;
}

void GraphicsPSO::SetBlendState(const D3D12_BLEND_DESC& blendDesc)
{
	m_psoDesc.BlendState = blendDesc;
}

void GraphicsPSO::SetRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc)
{
	m_psoDesc.RasterizerState = rasterizerDesc;
}

void GraphicsPSO::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc)
{
	m_psoDesc.DepthStencilState = depthStencilDesc;
}

void GraphicsPSO::SetNodeMask(UINT nodeMask) {
	m_psoDesc.NodeMask = nodeMask;
}

void GraphicsPSO::SetSampleMask(UINT sampleMask)
{
	m_psoDesc.SampleMask = sampleMask;
}

void GraphicsPSO::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
	m_psoDesc.PrimitiveTopologyType = topologyType;
}

void GraphicsPSO::SetDepthStencilFormat(DXGI_FORMAT DSVFormat, UINT massCount, UINT massQuality)
{
	SetRenderTargetFormats(0, nullptr, DSVFormat, massCount, massQuality);
}

void GraphicsPSO::SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT massCount, UINT massQuality)
{
	SetRenderTargetFormats(1, &RTVFormat, DSVFormat, massCount, massQuality);
}

void GraphicsPSO::SetRenderTargetFormats(UINT numRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT massCount, UINT massQuality)
{
	assert(numRTVs > 0 && RTVFormats != nullptr);
	for (int i = 0; i < numRTVs; ++i) {
		assert(RTVFormats[i] != DXGI_FORMAT_UNKNOWN);
		m_psoDesc.RTVFormats[i] = RTVFormats[i];
	}
	for (int i = numRTVs; i < m_psoDesc.NumRenderTargets; ++i) {
		m_psoDesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	m_psoDesc.NumRenderTargets = numRTVs;
	m_psoDesc.DSVFormat = DSVFormat;
	m_psoDesc.SampleDesc.Count = massCount;
	m_psoDesc.SampleDesc.Quality = massQuality;
}

void GraphicsPSO::SetInputLayout(UINT numElements, const D3D12_INPUT_ELEMENT_DESC* inputElements)
{
	m_psoDesc.InputLayout.NumElements = numElements;
	if (numElements > 0) {
		D3D12_INPUT_ELEMENT_DESC* newElements = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * numElements);
		memcpy(newElements, inputElements, sizeof(D3D12_INPUT_ELEMENT_DESC) * numElements);
		m_inputLayout.reset(newElements);
	}
	else {
		m_inputLayout = nullptr;
	}
}

void GraphicsPSO::Finalize()
{
	//set root signature
	m_psoDesc.pRootSignature = m_rootSignature->GetSignature();
	assert(m_psoDesc.pRootSignature != nullptr);
	//set input layout
	m_psoDesc.InputLayout.pInputElementDescs = m_inputLayout.get();
	
	ThrowIfFailed(GRAPHICS_CORE::g_device->CreateGraphicsPipelineState(&m_psoDesc, IID_PPV_ARGS(&m_pso)));
	m_pso->SetName(m_name);
}

/*
* ComputePSO
*/
ComputePSO::ComputePSO(const wchar_t* name) :PSO(name) {
	//Parameter initialization
	::ZeroMemory(&m_psoDesc, sizeof(m_psoDesc));
	m_psoDesc.NodeMask = 1;
}

void ComputePSO::Finalize()
{
	m_psoDesc.pRootSignature = m_rootSignature->GetSignature();
	assert(m_psoDesc.pRootSignature != nullptr);
	ThrowIfFailed(GRAPHICS_CORE::g_device->CreateComputePipelineState(&m_psoDesc, 
		IID_PPV_ARGS(&m_pso)));
	m_pso->SetName(m_name);
}
