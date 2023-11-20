#pragma once

#include "commandmanager.h"
#include "descriptorheapallocator.h"


namespace GRAPHICS_CORE
{
	extern CommandManager g_commandManager;

	extern ID3D12Device* g_device;
	extern bool g_tearingSupport;

	extern DescriptorAllocator g_descriptorHeapAllocator[];


	void GraphicsCoreInitialize();
	void GraphicsCoreRelease();

	D3D12_CPU_DESCRIPTOR_HANDLE AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count = 1);

}