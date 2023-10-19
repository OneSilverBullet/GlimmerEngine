#include "headers.h"


class RootSignature;

//Pipeline State Object Base Class
class PSO
{
public:
	PSO(const wchar_t* name): m_name(name),m_rootSignature(nullptr), m_pso(nullptr){}

	void SetRootSignature(RootSignature* rootSignature) { m_rootSignature = rootSignature; }
	const RootSignature* GetRootSignature() const { return m_rootSignature; }

	ID3D12PipelineState* GetPSO() const { return m_pso; }
protected:
	const wchar_t* m_name = nullptr;
	RootSignature* m_rootSignature = nullptr;
	ID3D12PipelineState* m_pso = nullptr;
};

//The PSO for rendering
class GraphicsPSO : public PSO
{
public:
	GraphicsPSO(const wchar_t* name = L"render_pso");

	//Set the parameters in Graphics PSO
	void SetBlendState(const D3D12_BLEND_DESC& blendDesc);
	void SetRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc);
	void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc);
	void SetNodeMask(UINT nodeMask);
    void SetSampleMask(UINT sampleMask);
	void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType);
	void SetDepthStencilFormat(DXGI_FORMAT DSVFormat, UINT massCount = 1, UINT massQuality = 0);
	void SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT massCount = 1, UINT massQuality = 0);
	void SetRenderTargetFormats(UINT numRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT massCount = 1, UINT massQuality = 0);
	void SetInputLayout(UINT numElements, const D3D12_INPUT_ELEMENT_DESC* inputElements);

	//Upload shader code
	void SetVertexShader(const void* binary, size_t size) { m_psoDesc.VS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(binary), size); }
	void SetVertexShader(const D3D12_SHADER_BYTECODE& binary) { m_psoDesc.VS = binary; }
	void SetPixelShader(const void* binary, size_t size) { m_psoDesc.PS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(binary), size); }
	void SetPixelShader(const D3D12_SHADER_BYTECODE& binary) { m_psoDesc.PS = binary; }
	void SetGeometryShader(const void* binary, size_t size) { m_psoDesc.GS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(binary), size); }
	void SetGeometryShader(const D3D12_SHADER_BYTECODE& binary) { m_psoDesc.GS = binary; }
	void SetHullShader(const void* binary, size_t size) { m_psoDesc.HS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(binary), size); }
	void SetHullShader(const D3D12_SHADER_BYTECODE& binary) { m_psoDesc.HS = binary; }
	void SetDomainShader(const void* binary, size_t size) { m_psoDesc.DS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(binary), size); }
	void SetDomainShader(const D3D12_SHADER_BYTECODE& binary) { m_psoDesc.DS = binary; }

	void Finalize();


private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_psoDesc = {};

	std::shared_ptr<const D3D12_INPUT_ELEMENT_DESC> m_inputLayout = nullptr;
};


//PSO for compute
class ComputePSO : public PSO
{
public:
	ComputePSO(const wchar_t* name = L"compute_pso");

	//Upload shader code
	void SetComputeShader(const void* binary, size_t size) { m_psoDesc.CS = CD3DX12_SHADER_BYTECODE(const_cast<void*>(binary), size); }
	void SetComputeShader(const D3D12_SHADER_BYTECODE& binary) { m_psoDesc.CS = binary; }

	void Finalize();

private:
	D3D12_COMPUTE_PIPELINE_STATE_DESC m_psoDesc = {};
};


