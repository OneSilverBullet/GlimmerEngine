#include "headers.h"


class RootParameter
{
public:
	RootParameter() {
		m_rootParam.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
	}

	~RootParameter()
	{
		Clear();
	}

	void Clear() {
		if (m_rootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
			delete[] m_rootParam.DescriptorTable.pDescriptorRanges;
		}
		m_rootParam.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
	}

	void InitAsConstant32(UINT registerSlot, UINT numDwords, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0)
	{
		m_rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		m_rootParam.ShaderVisibility = visibility;
		m_rootParam.Constants.Num32BitValues = numDwords;
		m_rootParam.Constants.RegisterSpace = space;
		m_rootParam.Constants.ShaderRegister = registerSlot;
	}

protected:
	D3D12_ROOT_PARAMETER m_rootParam;
};

class RootSignature
{
public:

	RootSignature(UINT rootParamNum = 0, UINT samplerNum = 0)
		: m_parametersNum(rootParamNum), m_samplersNum(samplerNum)
	{
		Reset(rootParamNum, samplerNum);
	}

	~RootSignature()
	{
	}

	void Reset(UINT rootParamNum, UINT samplerNum = 0)
	{
		if (rootParamNum > 0)
			m_parameters.reset(new RootParameter[rootParamNum]);
		else
			m_parameters = nullptr;
		m_parametersNum = rootParamNum;

		if (samplerNum > 0)
			m_samplers.reset(new D3D12_STATIC_SAMPLER_DESC[samplerNum]);
		else
			m_samplers = nullptr;
		m_samplersNum = samplerNum;
	}

	RootParameter& operator[](size_t index) {
		assert(index < m_parametersNum);
		return m_parameters.get()[index];
	}

	const RootParameter& operator[](size_t index) const {
		assert(index < m_parametersNum);
		return m_parameters.get()[index];
	}

	ID3D12RootSignature* GetSignature() { return m_rootSignature; }

	//compile and create root signature object
	void Finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);


protected:
	bool m_finalized = false;
	int m_parametersNum;
	int m_samplersNum;

	std::unique_ptr<RootParameter[]> m_parameters;
	std::unique_ptr<D3D12_STATIC_SAMPLER_DESC[]> m_samplers;
	ID3D12RootSignature* m_rootSignature;
};



