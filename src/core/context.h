#pragma once
#include <d3d12.h>
#include <vector>
#include <memory>
#include "commandmanager.h"
#include "dynamicdescriptorheap.h"
#include "commandmanager.h"
#include "resources/memoryallocator/linearallocator.h"
#include "resources/gpuresource.h"
#include "resources/readbackbuffer.h"
#include "resources/pixelbuffer.h"
#include "resources/colorbuffer.h"
#include "resources/depthbuffer.h"
#include "types/commontypes.h"
#include "pso.h"

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

//Central Context contains multiple static functions related to temp context
class CentralContext
{
	//initialize a texture
	static void InitializeTexture(GPUResource& dest, UINT numSubresources, D3D12_SUBRESOURCE_DATA subData[]);
	static void InitializeBuffer(GPUResource& dest, const void* data, size_t numBytes, size_t offset = 0);
	static void InitializeBuffer(GPUResource& dest, const UploadBuffer& src, size_t srcOffset, size_t destOffset);

};


//context class limitation
struct NonCopyable
{
	NonCopyable() = default;
	NonCopyable(const NonCopyable& v) = delete;
	NonCopyable& operator=(const NonCopyable& v) = delete;
};

//basic context class
class Context : public NonCopyable
{
	friend ContextManager;
	friend CommandQueue;
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

	//create a readback texture
	uint32_t ReadbackTexture(ReadbackBuffer& dstBuffer, PixelBuffer& pixelBuffer);

	//reserve the memory for upload buffer
	DynamicAlloc ReserverUploadMemory(size_t sizeUpload) {
		return m_cpuLinearAllocator.Allocate(sizeUpload);
	}


	//write or fill into the buffer/texture
	void WriteBuffer(GPUResource& dest, size_t destOffset, const void* data, size_t numBytes);
	void FillBuffer(GPUResource& dest, size_t destOffset, DWParam value, size_t numBytes);

	//texture status transition
	void TransitionResource(GPUResource& resource, D3D12_RESOURCE_STATES newState, bool flushImm = false);
	void InsertUAVBarrier(GPUResource& resource, bool flushImm = false);
	inline void FlushResourceBarrier();

	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heapPtr);
	void SetDescriptorHeaps(UINT heapCount, D3D12_DESCRIPTOR_HEAP_TYPE type[], ID3D12DescriptorHeap* heapPtrs[]);
	void SetPiplelineObject(const PSO& pso);


	D3D12_COMMAND_LIST_TYPE GetContextType() { return m_type; }

protected:

	void BindDescriptorHeaps();

	ID3D12CommandAllocator* m_commandAllocator;
	ID3D12GraphicsCommandList* m_graphicsCommandList;

	ID3D12RootSignature* m_graphicsSignature; //root signature for render shader
	ID3D12RootSignature* m_computeSignature; //root signature for compute shader
	ID3D12PipelineState* m_pipelineState; //render pipeline object

	DynamicDescriptorHeap m_dynamicViewDescriptorHeap; // HEAP_TYPE_CBV_SRV_UAV
	DynamicDescriptorHeap m_dynamicSamplerDescriptorHeap; // HEAP_TYPE_SAMPLER

	D3D12_RESOURCE_BARRIER m_resourceBarrierBuffer[16]; //store the resource barriers
	UINT m_numBarriersToFlush;

	ID3D12DescriptorHeap* m_currentDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	DynamicLinearMemoryAllocator m_cpuLinearAllocator;

	std::wstring m_ID;
	void setID(const std::wstring& id) { m_ID = id; }

	D3D12_COMMAND_LIST_TYPE m_type;
};

//the context for rendering
class GraphicsContext : public Context
{
public:

	void ClearUAV(GPUBuffer& buffer);
	void ClearUAV(ColorBuffer& buffer);
	void ClearColor(ColorBuffer& target, D3D12_RECT* rect = nullptr);
	void ClearColor(ColorBuffer& target, float colour[4], D3D12_RECT* rect = nullptr);
	void ClearDepth(DepthBuffer& target);
	void ClearStencil(DepthBuffer& target);
	void ClearDepthAndStencil(DepthBuffer& target);


	void SetRootSignature(const RootSignature& root);

	void SetRenderTargets(UINT numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[]);
	void SetRenderTargets(UINT numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV);
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv) { SetRenderTargets(1, &rtv); }
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv) { SetRenderTargets(1, &rtv, dsv); }
	void SetDepthAndStencil(D3D12_CPU_DESCRIPTOR_HANDLE dsv) { SetRenderTargets(0, nullptr, dsv); }

	void SetViewport(const D3D12_VIEWPORT& vp);
	void SetViewport(float x, float y, float w, float h, float minDepth = 0.0f, float maxDepth = 1.0f);

	void SetScissor(const D3D12_RECT rect);
	void SetScissor(UINT left, UINT right, UINT up, UINT bottom);






};

//the context for GPU computing
class ComputeContext : public Context
{


};


