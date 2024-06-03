#pragma once
#include "context.h"
#include "graphicscore.h"
/*
* ContextManager
*/
Context* ContextManager::AllocateContext(D3D12_COMMAND_LIST_TYPE type)
{
	std::lock_guard<std::mutex> lockGuard(sm_contextAllocatorMutex);

	auto& availableContexts = m_availableContextPool[type];

	Context* ret = nullptr;
	if (availableContexts.empty()) {
		ret = new Context(type);
		m_availableContextPool[type].push(ret);
		ret->Initialize();
	}
	else {
		ret = availableContexts.front();
		availableContexts.pop();
		ret->Reset();
	}

	return ret;
}

void ContextManager::FreeContext(Context* usedContext)
{
	std::lock_guard<std::mutex> lockGuard(sm_contextAllocatorMutex);
	m_availableContextPool[usedContext->GetContextType()].push(usedContext);
}

void ContextManager::DestroyAllContexts()
{
}


/*
* Context
*/
Context::Context(D3D12_COMMAND_LIST_TYPE type) :
	m_type(type),
	m_dynamicViewDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
	m_dynamicSamplerDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
	m_cpuLinearAllocator(CPU_MEMORY_ALLOCATION)
{
	m_graphicsCommandList = nullptr;
	m_commandAllocator = nullptr;
	ZeroMemory(m_currentDescriptorHeap, sizeof(m_currentDescriptorHeap));

	m_graphicsSignature = nullptr;
	m_computeSignature = nullptr;
	m_pipelineState = nullptr;
	m_numBarriersToFlush = 0;
}

void Context::Reset() {
	//make sure the command list has been initialized
	assert(m_graphicsCommandList != nullptr && m_commandAllocator == nullptr);
	m_commandAllocator = GRAPHICS_CORE::g_commandManager.GetQueue(m_type).RequestAllocator();
	m_graphicsCommandList->Reset(m_commandAllocator, nullptr);

	m_graphicsSignature = nullptr;
	m_computeSignature = nullptr;
	m_pipelineState = nullptr;
	m_numBarriersToFlush = 0;

	BindDescriptorHeaps();
}

Context::~Context() {
	if (m_graphicsCommandList != nullptr)
		m_graphicsCommandList->Release();
}

void Context::Initialize(void) {
	GRAPHICS_CORE::g_commandManager.CreateNewCommandList(m_type, &m_graphicsCommandList, &m_commandAllocator);
}

uint64_t Context::Flush(bool waitForCompletion) {
	FlushResourceBarrier();

	assert(m_commandAllocator != nullptr);

	//flush graphics command list
	uint64_t fenceValue = GRAPHICS_CORE::g_commandManager.GetQueue(m_type).ExecuteCommandList(m_graphicsCommandList);

	if (waitForCompletion)
		GRAPHICS_CORE::g_commandManager.WaitForFence(fenceValue);

	/*
	* Reset the command list and restore states
	*/
	m_graphicsCommandList->Reset(m_commandAllocator, nullptr);
	if (m_graphicsSignature)
		m_graphicsCommandList->SetGraphicsRootSignature(m_graphicsSignature);
	if (m_computeSignature)
		m_graphicsCommandList->SetComputeRootSignature(m_computeSignature);
	if (m_pipelineState)
		m_graphicsCommandList->SetPipelineState(m_pipelineState);

	BindDescriptorHeaps();
}

uint64_t Context::Finish(bool waitForCompletion) {

}

void Context::CopyBuffer(GPUResource& dest, GPUResource& src) {
	TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionResource(src, D3D12_RESOURCE_STATE_COPY_SOURCE);
	FlushResourceBarrier();
	//the core api invoction in dx12
	m_graphicsCommandList->CopyResource(dest.GetResource(), src.GetResource());
}

void Context::CopyBufferRegion(GPUResource& dest, size_t destOffset,
	GPUResource& src, size_t srcOffset, size_t numBytes) {
	TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionResource(src, D3D12_RESOURCE_STATE_COPY_SOURCE);
	FlushResourceBarrier();
	//the core api invocation in dx12
	m_graphicsCommandList->CopyBufferRegion(dest.GetResource(), destOffset,
		src.GetResource(), srcOffset, numBytes);
}

void Context::CopySubResource(GPUResource& dest, size_t destIndex,
	GPUResource& src, size_t srcIndex) {
	FlushResourceBarrier();

	D3D12_TEXTURE_COPY_LOCATION destLocation;
	destLocation.pResource = dest.GetResource();
	destLocation.SubresourceIndex = destIndex;
	destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	D3D12_TEXTURE_COPY_LOCATION srcLocation;
	srcLocation.pResource = src.GetResource();
	srcLocation.SubresourceIndex = srcIndex;
	srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	//the core api invocation in dx12
	m_graphicsCommandList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
}

uint32_t Context::ReadbackTexture(ReadbackBuffer& dstBuffer, PixelBuffer& srcBuffer) {

	uint64_t copySize = 0;

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedFootprint;
	GRAPHICS_CORE::g_device->GetCopyableFootprints(&srcBuffer.GetResource()->GetDesc(), 0, 1, 0,
		&placedFootprint, nullptr, nullptr, &copySize);

	//create a read back buffer to prepare to store the data from GPU buffer
	dstBuffer.Create(L"Readback", (uint32_t)copySize, 1);

	TransitionResource(srcBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE);
	FlushResourceBarrier();

	//copy the texture region from the src to the dest
	m_graphicsCommandList->CopyTextureRegion(
		&CD3DX12_TEXTURE_COPY_LOCATION(dstBuffer.GetResource(), placedFootprint), 0, 0, 0,
		&CD3DX12_TEXTURE_COPY_LOCATION(srcBuffer.GetResource(), 0), nullptr);

	//return the row pitch 
	return placedFootprint.Footprint.RowPitch;
}


void Context::WriteBuffer(GPUResource& dest, size_t destOffset,
	const void* data, size_t numBytes) {

}

void Context::FillBuffer(GPUResource& dest, size_t destOffset, 
	DWParam value, size_t numBytes) {

}

void Context::TransitionResource(GPUResource& resource,
	D3D12_RESOURCE_STATES newState, bool flushImm) {

	D3D12_RESOURCE_STATES oldState = resource.GetUsageState();

	if (oldState != newState) {
		//D3D12_RESOURCE_BARRIER store the transition parameters 
		D3D12_RESOURCE_BARRIER& curResourceBarrier = m_resourceBarrierBuffer[m_numBarriersToFlush++];

		curResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		curResourceBarrier.Transition.pResource = resource.GetResource();
		curResourceBarrier.Transition.StateAfter = newState;
		curResourceBarrier.Transition.StateBefore = oldState;
		curResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		resource.SetUsageState(newState);
	}
	else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		InsertUAVBarrier(resource, flushImm);

	//once the resource barrier command is equal to 16, we flush the resource barrier immediately
	if (flushImm || m_numBarriersToFlush == 16)
		FlushResourceBarrier();
}

void Context::InsertUAVBarrier(GPUResource& resource, bool flushImm) {
	D3D12_RESOURCE_BARRIER& curBarrier = m_resourceBarrierBuffer[m_numBarriersToFlush++];
	curBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	curBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	curBarrier.UAV.pResource = resource.GetResource();

	if (flushImm)
		FlushResourceBarrier();
}

void Context::FlushResourceBarrier() {
	if (m_numBarriersToFlush > 0) {
		//transmit the resource barrier into the graphics command list and start processing
		m_graphicsCommandList->ResourceBarrier(m_numBarriersToFlush, m_resourceBarrierBuffer);
		m_numBarriersToFlush = 0;
	}
}

void Context::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heapPtr) {
	if (m_currentDescriptorHeap[type] != heapPtr) {
		m_currentDescriptorHeap[type] = heapPtr;
		BindDescriptorHeaps();
	}
}

void Context::SetDescriptorHeaps(UINT heapCount, D3D12_DESCRIPTOR_HEAP_TYPE type[],
	ID3D12DescriptorHeap* heapPtrs[]) {
	bool changed = false;
	for (int i = 0; i < heapCount; ++i) {
		if (m_currentDescriptorHeap[type[i]] != heapPtrs[type[i]])
			m_currentDescriptorHeap[type[i]] = heapPtrs[type[i]];
	}
	if (changed)
		BindDescriptorHeaps();
}

void Context::SetPiplelineObject(const PSO& pso) {
	ID3D12PipelineState* newState = pso.GetPSO();
	if (newState == m_pipelineState)
		return;

	m_graphicsCommandList->SetPipelineState(newState);
	m_pipelineState = newState;
}

void Context::BindDescriptorHeaps() {
	UINT nonNullHeaps = 0;
	ID3D12DescriptorHeap* heapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	//fill the descriptor heaps countainer
	for (UINT i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
		ID3D12DescriptorHeap* heapIter = m_currentDescriptorHeap[i];
		if (heapIter != nullptr)
			heapsToBind[nonNullHeaps++] = heapIter;
	}

	//bind the heaps to current graphics command list
	if (nonNullHeaps > 0)
		m_graphicsCommandList->SetDescriptorHeaps(nonNullHeaps, heapsToBind);
}

void CentralContext::InitializeTexture(GPUResource& dest, UINT numSubresources, D3D12_SUBRESOURCE_DATA subData[])
{
}

void CentralContext::InitializeBuffer(GPUResource& dest, const void* data, size_t numBytes, size_t offset)
{
}

void CentralContext::InitializeBuffer(GPUResource& dest, const UploadBuffer& src, size_t srcOffset, size_t destOffset)
{
}
