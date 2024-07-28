#include "mipmapgenerator.h"
#include "resources/colorbuffer.h"
#include "context.h"
#include "graphicscore.h"


void MipmapGenerator::Initialize() {
	InitializeRS();
	InitializePSO();
}

void MipmapGenerator::GenerateMipmap(ColorBuffer* colorbuffer)
{
	Context* context = GRAPHICS_CORE::g_contextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	ComputeContext& computeContext = context->GetComputeContext();

	computeContext.TransitionResource(*colorbuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	computeContext.SetDynamicDescriptor(1, 0, colorbuffer->GetSRV());

	uint32_t numMipmaps =  colorbuffer->GetMipsMap();
	uint32_t originWidth = colorbuffer->GetWidth();
	uint32_t originHeight = colorbuffer->GetHeight();

	for (uint32_t i = 0; i < numMipmaps; ++i) {
		uint32_t srcWidth = originWidth >> i;
		uint32_t srcHeight = originHeight >> i;
		uint32_t dstWidth = srcWidth >> 1;
		uint32_t dstHeight = srcHeight >> 1;

		//set the mip map to the compute context
		uint32_t mipType = (srcWidth & 1) | (srcHeight & 1) << 1;
		computeContext.SetPiplelineObject(m_psos[mipType]);

		computeContext.SetConstants(0, i, 1.0f / (float)dstWidth, 1.0f / (float)dstHeight);

		D3D12_CPU_DESCRIPTOR_HANDLE uav = colorbuffer->GetUAV(i);

		computeContext.SetDynamicDescriptor(2, 0, uav);
		computeContext.Dispatch2D(dstWidth, dstHeight);

		computeContext.InsertUAVBarrier(*colorbuffer);
	}

	computeContext.TransitionResource(*colorbuffer, 
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void MipmapGenerator::InitializeRS()
{
	m_rootSig.Reset(4, 3);
	m_rootSig[0].InitAsConstant32(0, 4);
	m_rootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10);
	m_rootSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 10);
	m_rootSig[3].InitAsConstantBuffer(1);
	m_rootSig.InitSamplerDesc(0, GRAPHICS_CORE::g_samplerLinearWrapDesc);
	m_rootSig.InitSamplerDesc(1, GRAPHICS_CORE::g_samplerPointBorderDesc);
	m_rootSig.InitSamplerDesc(2, GRAPHICS_CORE::g_samplerLinearBorderDesc);
	m_rootSig.Finalize(L"Computer Shader Commmon");
}


void MipmapGenerator::InitializePSO() {
	ComPtr<ID3DBlob> mipmapcsXODD;
	ThrowIfFailed(D3DReadFileToBlob(L"mipmapcs_xodd.cso", &mipmapcsXODD));
	ComPtr<ID3DBlob> mipmapcsYODD;
	ThrowIfFailed(D3DReadFileToBlob(L"mipmapcs_yodd.cso", &mipmapcsYODD));
	ComPtr<ID3DBlob> mipmapcsXYODD;
	ThrowIfFailed(D3DReadFileToBlob(L"mipmapcs_xyodd.cso", &mipmapcsXYODD));
	ComPtr<ID3DBlob> mipmapcsXYEVEN;
	ThrowIfFailed(D3DReadFileToBlob(L"mipmapcs_xyeven.cso", &mipmapcsXYEVEN));

	m_psos[(int)MipmapType::XODD].SetComputeShader(mipmapcsXODD->GetBufferPointer(), mipmapcsXODD->GetBufferSize());
	m_psos[(int)MipmapType::XODD].SetRootSignature(&m_rootSig);
	m_psos[(int)MipmapType::YODD].SetComputeShader(mipmapcsYODD->GetBufferPointer(), mipmapcsYODD->GetBufferSize());
	m_psos[(int)MipmapType::YODD].SetRootSignature(&m_rootSig);
	m_psos[(int)MipmapType::XYODD].SetComputeShader(mipmapcsXYODD->GetBufferPointer(), mipmapcsXYODD->GetBufferSize());
	m_psos[(int)MipmapType::XYODD].SetRootSignature(&m_rootSig);
	m_psos[(int)MipmapType::XYEVEN].SetComputeShader(mipmapcsXYEVEN->GetBufferPointer(), mipmapcsXYEVEN->GetBufferSize());
	m_psos[(int)MipmapType::XYEVEN].SetRootSignature(&m_rootSig);
}
