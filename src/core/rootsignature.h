#include "headers.h"
#include <map>
#include <wrl/client.h>

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

	const D3D12_ROOT_PARAMETER& operator()(void) { return m_rootParam; }

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

	void InitAsConstantBuffer(UINT registerSlot, D3D12_SHADER_VISIBILITY visible = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0)
	{
		m_rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; //create constant buffer root parameter
		m_rootParam.ShaderVisibility = visible;
		m_rootParam.Descriptor.RegisterSpace = space;
		m_rootParam.Descriptor.ShaderRegister = registerSlot;
	}

	void InitAsBufferSRV(UINT registerSlot, D3D12_SHADER_VISIBILITY visible = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0)
	{
		m_rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV; //create shader resource root parameter
		m_rootParam.ShaderVisibility = visible;
		m_rootParam.Descriptor.RegisterSpace = space;
		m_rootParam.Descriptor.ShaderRegister = registerSlot;
	}

	void InitAsBufferUAV(UINT registerSlot, D3D12_SHADER_VISIBILITY visible = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0)
	{
		m_rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV; //create unordered access root parameter
		m_rootParam.ShaderVisibility = visible;
		m_rootParam.Descriptor.RegisterSpace = space;
		m_rootParam.Descriptor.ShaderRegister = registerSlot;
	}

	void InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE type, UINT registerSlot,
		UINT count, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0) {
		//we just use one descriptors range in descriptor table
		InitAsDescriptorTable(1, visibility, space);
		SetTableRange(0, type, registerSlot, count, space);
	}

	void InitAsDescriptorTable(UINT registerSlotRange, D3D12_SHADER_VISIBILITY visible = D3D12_SHADER_VISIBILITY_ALL, UINT space = 0)
	{
		m_rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		m_rootParam.ShaderVisibility = visible;
		m_rootParam.DescriptorTable.NumDescriptorRanges = registerSlotRange;
		m_rootParam.DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[registerSlotRange];
	}

	void SetTableRange(UINT rangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE type, UINT registerSlot, UINT count, UINT space = 0)
	{
		D3D12_DESCRIPTOR_RANGE* range = const_cast<D3D12_DESCRIPTOR_RANGE*>(m_rootParam.DescriptorTable.pDescriptorRanges + rangeIndex);
		range->RangeType = type;
		range->NumDescriptors = count;
		range->RegisterSpace = space;
		range->BaseShaderRegister = registerSlot;
		range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}

protected:
	D3D12_ROOT_PARAMETER m_rootParam;
};

class RootSignatureManager
{
public:
	static RootSignatureManager& Instance() {
		static RootSignatureManager instance;
		return instance;
	}

	void Insert(uint32_t hashValue, ID3D12RootSignature* rootSignatureCompiled);
	ID3D12RootSignature* Get(uint32_t hashValue);
	void Release();

private:
	RootSignatureManager(){}
	RootSignatureManager(const RootSignatureManager& val) = delete;
	RootSignatureManager& operator=(const RootSignatureManager& val) = delete;
		 
private:
	std::map<uint32_t, Microsoft::WRL::ComPtr<ID3D12RootSignature>> m_storage;
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

	ID3D12RootSignature* GetSignature() const { return m_rootSignature; }

	//compile and create root signature object
	void Finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

public:
	uint32_t m_descriptorTableBitMap; //to indicate the descriptor table slot index
	uint32_t m_descriptorTableSize[16]; //record the descriptors count in each descriptor table
	uint32_t m_samplerBitMap; //to indicate the sampler slot index

protected:
	bool m_finalized = false;
	int m_parametersNum;
	int m_samplersNum;
	std::unique_ptr<RootParameter[]> m_parameters;
	std::unique_ptr<D3D12_STATIC_SAMPLER_DESC[]> m_samplers;
	ID3D12RootSignature* m_rootSignature;
};



