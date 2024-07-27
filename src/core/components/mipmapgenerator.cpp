#include "mipmapgenerator.h"
#include "resources/colorbuffer.h"
#include "context.h"
#include "graphicscore.h"




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


	}


}

void MipmapGenerator::InitializeRS()
{
	m_rootSig.Reset(4, 0);
	m_rootSig[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
	m_rootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 10);
	m_rootSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 10);
	m_rootSig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0, 10);
	m_rootSig.Finalize(L"Computer Shader Commmon");
}
