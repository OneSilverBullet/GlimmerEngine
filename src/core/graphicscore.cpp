#include "graphicscore.h"
#include "headers.h"
#include "descriptorheapallocator.h"

namespace GRAPHICS_CORE
{
	CommandManager g_commandManager;

	DescriptorAllocator g_descriptorHeapAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	};

	D3D12_CPU_DESCRIPTOR_HANDLE AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count)
	{
		return g_descriptorHeapAllocator[type].Allocator(count);
	}

	void GraphicsCoreInitialize()
	{

	}
}