#include "pso.h"
#include "rootsignature.h"

/*
* GraphcisPSO
*/
GraphcisPSO::GraphcisPSO(const wchar_t* name) :PSO(name) {
	//Parameter initialization
	::ZeroMemory(&m_psoDesc, sizeof(m_psoDesc));
	m_psoDesc.NodeMask = 0;
	m_psoDesc.SampleMask = 0xFFFFFFFFu;
	m_psoDesc.SampleDesc.Count = 1;
	m_psoDesc.InputLayout.NumElements = 0;
}

void GraphcisPSO::SetBlendState(const D3D12_BLEND_DESC& blendDesc)
{
	m_psoDesc.BlendState = blendDesc;
}

void GraphcisPSO::SetRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc)
{
	m_psoDesc.RasterizerState = rasterizerDesc;
}

void GraphcisPSO::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc)
{
	m_psoDesc.DepthStencilState = depthStencilDesc;
}

void GraphcisPSO::SetSampleMask(UINT sampleMask)
{
	m_psoDesc.SampleMask = sampleMask;
}

void GraphcisPSO::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
{
	m_psoDesc.PrimitiveTopologyType = topologyType;
}

void GraphcisPSO::SetDepthStencilFormat(DXGI_FORMAT DSVFormat, UINT massCount, UINT massQuality)
{
	SetRenderTargetFormats(0, nullptr, DSVFormat, massCount, massQuality);
}

void GraphcisPSO::SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT massCount, UINT massQuality)
{
	SetRenderTargetFormats(1, &RTVFormat, DSVFormat, massCount, massQuality);
}

void GraphcisPSO::SetRenderTargetFormats(UINT numRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT massCount, UINT massQuality)
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

void GraphcisPSO::SetInputLayout(UINT numElements, const D3D12_INPUT_ELEMENT_DESC* inputElements)
{
}

void GraphcisPSO::Finalize()
{
}

/*
* ComputePSO
*/
void ComputePSO::Finalize()
{
}
