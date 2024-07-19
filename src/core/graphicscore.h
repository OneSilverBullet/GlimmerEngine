#pragma once

#include <string>
#include "commandmanager.h"
#include "context.h"
#include "descriptorheapallocator.h"
#include "texturemanager.h"
#include "resources/samplerdesc.h"
#include "geometry/model.h"
#include "geometry/material.h"
#include "staticdecriptorheap.h"

namespace GRAPHICS_CORE
{
	extern TextureManager g_textureManager;
	extern CommandManager g_commandManager;
	extern ContextManager g_contextManager;
	extern StaticDescriptorHeap g_texturesDescriptorHeap;
	extern StaticDescriptorHeap g_samplersDescriptorHeap;
	extern ModelManager g_staticModelsManager;
	extern MaterialManager g_materialManager;

	extern ID3D12Device* g_device;
	extern bool g_tearingSupport;

	extern DescriptorAllocator g_descriptorHeapAllocator[];


	//resource pathes
	extern std::string g_texturePath;
	extern std::string g_pbrmaterialTextureName[5];


	//global default sampler descriptor handle
	//todo: build up samplers manager
	extern SamplerDesc g_samplerLinearWrapDesc;
	extern D3D12_CPU_DESCRIPTOR_HANDLE g_samplerLinearWrap;
	extern SamplerDesc g_samplerAnisoWrapDesc;
	extern D3D12_CPU_DESCRIPTOR_HANDLE g_samplerAnisoWrap;


	void GraphicsCoreInitialize();
	void GraphicsCoreRelease();

	D3D12_CPU_DESCRIPTOR_HANDLE AllocatorDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT count = 1);
	UINT32 GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type);
	UINT GetDXGIFormatSize(DXGI_FORMAT format);

}