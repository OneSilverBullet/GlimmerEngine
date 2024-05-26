#pragma once
#include <d3d12.h>
#include <vector>
#include <memory>
#include "commandmanager.h"
#include "dynamicdescriptorheap.h"
#include "resources/memoryallocator/linearallocator.h"
#include "resources/gpuresource.h"

class Context;

class ContextManager
{
public:
	ContextManager(){}

	Context* AllocateContext(D3D12_COMMAND_LIST_TYPE type);
	void FreeContext(Context*);
	void DestroyAllContexts();

private:
	std::vector<std::unique_ptr<Context>> m_contextPool[4];
	std::queue<Context*> m_availableContextPool[4];
	std::mutex sm_contextAllocatorMutex;
};

//context class limitation
struct NonCopyable
{
	NonCopyable() = delete;
	NonCopyable(const NonCopyable& v) = delete;
	NonCopyable& operator=(const NonCopyable& v) = delete;
};

//basic context class
class Context : public NonCopyable
{
	friend ContextManager;
private:  //only context manager can generate a context
	Context(D3D12_COMMAND_LIST_TYPE type);
	void Reset();

public:

	~Context();

	//flush existing commands to the GPU but keep the context alive
	uint64_t Flush(bool waitForCompletion = false);

	//flush existing commands and release the current context
	uint64_t Finish(bool waitForCompletion = false);

	//reserving a command list and command allocator
	void Initialize(void);

	GraphicsContext& GetGraphicsContext() {
		return reinterpret_cast<GraphicsContext&>(*this);
	}

	ComputeContext& GetComputeContext() {
		return reinterpret_cast<ComputeContext&>(*this);
	}

	ID3D12CommandList* GetCommandList() { return m_graphicsCommandList; }

	//interface to copy texture
	void CopyBuffer(GPUResource& dest, GPUResource& src);
	void CopyBufferRegion(GPUResource& dest, size_t destOffset, GPUResource& src, size_t srcOffset, size_t numerBytes);
	void CopySubResource(GPUResource& dest, size_t destIndex, GPUResource& src, size_t srcIndex);




	  

protected:
	ID3D12CommandAllocator* m_commandAllocator;
	ID3D12GraphicsCommandList* m_graphicsCommandList;

	ID3D12RootSignature* m_renderSignature; //root signature for render shader
	ID3D12RootSignature* m_computeSignature; //root signature for compute shader
	ID3D12PipelineState* m_pipelineState; //render pipeline object

	DynamicDescriptorHeap m_dynamicViewDescriptorHeap; // HEAP_TYPE_CBV_SRV_UAV
	DynamicDescriptorHeap m_dynamicSamplerDescriptorHeap; // HEAP_TYPE_SAMPLER

	D3D12_RESOURCE_BARRIER m_resourceBarrierBuffer[16];
	UINT m_numBarriersToFlush;

	ID3D12DescriptorHeap* m_currentDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	DynamicLinearMemoryAllocator m_cpuLinearAllocator;

	std::wstring m_ID;
	void setID(const std::wstring& id) { m_ID = id; }

	D3D12_COMMAND_LIST_TYPE m_type;
};


class GraphicsContext : public Context
{



};


class ComputeContext : public Context
{


};


