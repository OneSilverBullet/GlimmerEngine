#pragma once
#include "context.h"
#include "graphicscore.h"
#include "rootsignature.h"
#include "mathematics/bitoperation.h"
#include "mathematics/bitoperation.h"
#include "d3dx12.h"


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

Context& ContextManager::GetAvailableContext() {
	Context* newContext = AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
	return *newContext;
}

GraphicsContext& ContextManager::GetAvailableGraphicsContext() {
	Context* newContext = AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
	return newContext->GetGraphicsContext();
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
	return fenceValue;
}

uint64_t Context::Finish(bool waitForCompletion) {

	FlushResourceBarrier();

	assert(m_commandAllocator != nullptr);

	CommandQueue& queue = GRAPHICS_CORE::g_commandManager.GetDirectQueue();

	uint64_t fenceValue = queue.ExecuteCommandList(m_graphicsCommandList);
	queue.DiscardCommandAllocator(fenceValue, m_commandAllocator);
	m_commandAllocator = nullptr;

	m_cpuLinearAllocator.ClearUpPages(fenceValue);
	m_dynamicViewDescriptorHeap.CleanupUsedHeap(fenceValue);
	m_dynamicSamplerDescriptorHeap.CleanupUsedHeap(fenceValue);

	if (waitForCompletion)
		GRAPHICS_CORE::g_commandManager.WaitForFence(fenceValue);

	GRAPHICS_CORE::g_contextManager.FreeContext(this);

	return fenceValue;
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

void Context::TransitionResource(GPUResource& resource, D3D12_RESOURCE_STATES oldState, D3D12_RESOURCE_STATES newState, bool flushImm) {
	if (oldState != newState) {
		//D3D12_RESOURCE_BARRIER store the transition parameters 
		D3D12_RESOURCE_BARRIER& curResourceBarrier = m_resourceBarrierBuffer[m_numBarriersToFlush++];

		curResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		curResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
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

void Context::TransitionResource(GPUResource& resource,
	D3D12_RESOURCE_STATES newState, bool flushImm) {
	D3D12_RESOURCE_STATES oldState = resource.GetUsageState();
	TransitionResource(resource, oldState, newState, flushImm);
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

void GlobalContext::InitializeTexture(GPUResource& dest, UINT numSubresources, D3D12_SUBRESOURCE_DATA subData[])
{
	//copy the texture data to the gpu resource
	Context& initContext = GRAPHICS_CORE::g_contextManager.GetAvailableContext();
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(dest.GetResource(), 0, numSubresources);

	D3D12_RESOURCE_DESC textureDesc =  dest.GetResource()->GetDesc();

	DynamicAlloc uploadBufferMem = initContext.ReserverUploadMemory(uploadBufferSize);
	UpdateSubresources((ID3D12GraphicsCommandList*)initContext.GetCommandList(), dest.GetResource(), uploadBufferMem.m_resource.GetResource(),
		0, 0, numSubresources, subData);
	initContext.TransitionResource(dest, D3D12_RESOURCE_STATE_GENERIC_READ);

	initContext.Finish(true);
}

void GlobalContext::InitializeBuffer(GPUResource& dest, const void* data, size_t numBytes, size_t offset)
{
	Context& initContext = GRAPHICS_CORE::g_contextManager.GetAvailableContext();
	
	DynamicAlloc uploadBufferMem = initContext.ReserverUploadMemory(153344);
	memcpy(uploadBufferMem.m_cpuVirtualAddress, data, Mathematics::DivideByMultiple(numBytes, 16));

	initContext.TransitionResource(dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
	initContext.GetGraphicCommandList()->CopyBufferRegion(dest.GetResource(), offset, uploadBufferMem.m_resource.GetResource(), 0, numBytes);
	initContext.TransitionResource(dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

	initContext.Finish();
}

void GlobalContext::InitializeBuffer(GPUResource& dest, const UploadBuffer& src, size_t srcOffset, size_t destOffset)
{
}


/*
* GraphicsContext: the context for rendering
*/
void GraphicsContext::ClearUAV(GPUBuffer& buffer)
{
	//finish all the resource transition
	FlushResourceBarrier();

	//after binding the buffer, we can get the gpu descriptor handle and we can clear the buffer by this handle
	D3D12_GPU_DESCRIPTOR_HANDLE gpuVisibleHandle = m_dynamicViewDescriptorHeap.UploadDirect(buffer.GetUAV());
	const UINT clearColor[4] = {};
	m_graphicsCommandList->ClearUnorderedAccessViewUint(gpuVisibleHandle, buffer.GetUAV(),
		buffer.GetResource(), clearColor, 0, nullptr);
}

void GraphicsContext::ClearUAV(ColorBuffer& buffer)
{
	FlushResourceBarrier();

	D3D12_GPU_DESCRIPTOR_HANDLE gpuVisibleHandle = m_dynamicViewDescriptorHeap.UploadDirect(buffer.GetUAV());
	CD3DX12_RECT clearRect(0, 0, (UINT)buffer.GetWidth(), (UINT)buffer.GetHeight());

	const float* clearColor = buffer.GetClearColor().GetPtr();
	m_graphicsCommandList->ClearUnorderedAccessViewFloat(gpuVisibleHandle, buffer.GetUAV(),
		buffer.GetResource(), clearColor, 1, &clearRect);

}

void GraphicsContext::ClearColor(ColorBuffer& buffer, D3D12_RECT* rect)
{
	FlushResourceBarrier();
	m_graphicsCommandList->ClearRenderTargetView(buffer.GetRTV(), buffer.GetClearColor().GetPtr(), (rect == nullptr) ? 0 : 1, rect);
}

void GraphicsContext::ClearColor(D3D12_CPU_DESCRIPTOR_HANDLE target, float colour[4]) {
	FlushResourceBarrier();
	m_graphicsCommandList->ClearRenderTargetView(target, colour, 0, nullptr);
}

void GraphicsContext::ClearColor(ColorBuffer& target, float colour[4], D3D12_RECT* rect)
{
	FlushResourceBarrier();
	m_graphicsCommandList->ClearRenderTargetView(target.GetRTV(), colour, (rect == nullptr) ? 1 : 0, rect);
}

void GraphicsContext::ClearDepth(DepthBuffer& target)
{
	FlushResourceBarrier();
	m_graphicsCommandList->ClearDepthStencilView(target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH,
		target.GetClearValue(), target.GetStencilValue(), 0, nullptr);
}

void GraphicsContext::ClearStencil(DepthBuffer& target)
{
	FlushResourceBarrier();
	m_graphicsCommandList->ClearDepthStencilView(target.GetDSV(), D3D12_CLEAR_FLAG_STENCIL,
		target.GetClearValue(), target.GetStencilValue(), 0, nullptr);
}

void GraphicsContext::ClearDepthAndStencil(DepthBuffer& target)
{
	FlushResourceBarrier();
	m_graphicsCommandList->ClearDepthStencilView(target.GetDSV(), D3D12_CLEAR_FLAG_STENCIL | D3D12_CLEAR_FLAG_DEPTH,
		target.GetClearValue(), target.GetStencilValue(), 0, nullptr);
}

void GraphicsContext::SetRootSignature(const RootSignature& rootSig)
{
	if (rootSig.GetSignature() == m_graphicsSignature)
		return;

	//bind the root signature 
	m_graphicsSignature = rootSig.GetSignature();
	m_graphicsCommandList->SetGraphicsRootSignature(m_graphicsSignature);

	//parse the root signature
	m_dynamicViewDescriptorHeap.ParseGraphicsRootSignature(rootSig);
	m_dynamicSamplerDescriptorHeap.ParseGraphicsRootSignature(rootSig);
}

void GraphicsContext::SetRenderTargets(UINT numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[])
{
	m_graphicsCommandList->OMSetRenderTargets(numRTVs, RTVs, FALSE, nullptr);
}

void GraphicsContext::SetRenderTargets(UINT numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{
	m_graphicsCommandList->OMSetRenderTargets(numRTVs, RTVs, FALSE, &DSV);
}

void GraphicsContext::SetViewport(const D3D12_VIEWPORT& vp)
{
	m_graphicsCommandList->RSSetViewports(1, &vp);
}

void GraphicsContext::SetViewport(float x, float y, float w, float h, float minDepth, float maxDepth)
{
	D3D12_VIEWPORT vp;
	vp.Width = w;
	vp.Height = h;
	vp.MinDepth = minDepth;
	vp.MaxDepth = maxDepth;
	vp.TopLeftX = x;
	vp.TopLeftY = y;
	m_graphicsCommandList->RSSetViewports(1, &vp);

}

void GraphicsContext::SetScissor(const D3D12_RECT rect)
{
	m_graphicsCommandList->RSSetScissorRects(1, &rect);
}

void GraphicsContext::SetScissor(UINT left, UINT right, UINT up, UINT bottom)
{
	D3D12_RECT rect;
	rect.left = left;
	rect.right = right;
	rect.top = up;
	rect.bottom = bottom;
	m_graphicsCommandList->RSSetScissorRects(1, &rect);
}

void GraphicsContext::SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect)
{
	SetViewport(vp);
	SetScissor(rect);
}

void GraphicsContext::SetStencilRef(UINT StencilRef)
{
	m_graphicsCommandList->OMSetStencilRef(StencilRef);
}

void GraphicsContext::SetBlendFactor(Color blendFactor)
{
	m_graphicsCommandList->OMSetBlendFactor(blendFactor.GetPtr());
}

void GraphicsContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
{
	m_graphicsCommandList->IASetPrimitiveTopology(topology);
}

void GraphicsContext::SetConstantArray(UINT rootIndex, UINT numConstants, const void* pConstants)
{
	m_graphicsCommandList->SetGraphicsRoot32BitConstants(rootIndex, numConstants, pConstants, 0);
}

void GraphicsContext::SetConstant(UINT rootIndex, UINT offset, DWParam val)
{
	m_graphicsCommandList->SetGraphicsRoot32BitConstant(rootIndex, val.Uint, offset);
}

void GraphicsContext::SetConstants(UINT rootIndex, DWParam X)
{
	m_graphicsCommandList->SetGraphicsRoot32BitConstant(rootIndex, X.Uint, 0);
}

void GraphicsContext::SetConstants(UINT rootIndex, DWParam X, DWParam Y)
{
	m_graphicsCommandList->SetGraphicsRoot32BitConstant(rootIndex, X.Uint, 0);
	m_graphicsCommandList->SetGraphicsRoot32BitConstant(rootIndex, Y.Uint, 1);
}

void GraphicsContext::SetConstants(UINT rootIndex, DWParam X, DWParam Y, DWParam Z)
{
	m_graphicsCommandList->SetGraphicsRoot32BitConstant(rootIndex, X.Uint, 0);
	m_graphicsCommandList->SetGraphicsRoot32BitConstant(rootIndex, Y.Uint, 1);
	m_graphicsCommandList->SetGraphicsRoot32BitConstant(rootIndex, Z.Uint, 2);
}

void GraphicsContext::SetConstants(UINT rootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
{
	m_graphicsCommandList->SetGraphicsRoot32BitConstant(rootIndex, X.Uint, 0);
	m_graphicsCommandList->SetGraphicsRoot32BitConstant(rootIndex, Y.Uint, 1);
	m_graphicsCommandList->SetGraphicsRoot32BitConstant(rootIndex, Z.Uint, 2);
	m_graphicsCommandList->SetGraphicsRoot32BitConstant(rootIndex, W.Uint, 3);
}

void GraphicsContext::SetConstantBuffer(UINT rootIndex, D3D12_GPU_VIRTUAL_ADDRESS cbv)
{
	m_graphicsCommandList->SetGraphicsRootConstantBufferView(rootIndex, cbv);
}

void GraphicsContext::SetDynamicConstantBufferView(UINT rootIndex, size_t bufferSize, 
	const void* bufferData)
{
	//allocate the constant buffer
	DynamicAlloc cb = m_cpuLinearAllocator.Allocate(bufferSize);
	memcpy(cb.m_cpuVirtualAddress, bufferData, bufferSize);
	//binding the constant buffer view 
	m_graphicsCommandList->SetGraphicsRootConstantBufferView(rootIndex, cb.m_gpuVirtualAddress);
}

void GraphicsContext::SetBufferSRV(UINT rootIndex, const GPUBuffer& srv, UINT64 offset)
{
	m_graphicsCommandList->SetGraphicsRootShaderResourceView(rootIndex, srv.RootConstantBufferView() + offset);
}

void GraphicsContext::SetBufferUAV(UINT rootIndex, const GPUBuffer& uav, UINT64 offset)
{
	m_graphicsCommandList->SetGraphicsRootShaderResourceView(rootIndex, uav.RootConstantBufferView() + offset);
}

void GraphicsContext::SetDescriptorTable(UINT rootIndex, D3D12_GPU_DESCRIPTOR_HANDLE firstHandle)
{
	m_graphicsCommandList->SetGraphicsRootDescriptorTable(rootIndex, firstHandle);
}

void GraphicsContext::SetDynamicDescriptor(UINT rootIndex, UINT offset, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	SetDynamicDescriptors(rootIndex, offset, 1, &handle);
}

void GraphicsContext::SetDynamicDescriptors(UINT rootIndex, UINT offset, UINT count, const D3D12_CPU_DESCRIPTOR_HANDLE handles[])
{
	m_dynamicViewDescriptorHeap.SetGraphicsDescriptorHandles(rootIndex, offset, count, handles);
}

void GraphicsContext::SetDynamicSampler(UINT rootIndex, UINT offset, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	SetDynamicSamplers(rootIndex, offset, 1, &handle);
}

void GraphicsContext::SetDynamicSamplers(UINT rootIndex, UINT offset, UINT count, const D3D12_CPU_DESCRIPTOR_HANDLE handles[])
{
	m_dynamicSamplerDescriptorHeap.SetGraphicsDescriptorHandles(rootIndex, offset, count, handles);
}

void GraphicsContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& ibView)
{
	m_graphicsCommandList->IASetIndexBuffer(&ibView);
}

void GraphicsContext::SetVertexBuffer(UINT slot, const D3D12_VERTEX_BUFFER_VIEW& vbView)
{
	SetVertexBuffers(slot, 1, &vbView);
}

void GraphicsContext::SetVertexBuffers(UINT startSlot, UINT count, const D3D12_VERTEX_BUFFER_VIEW vbViews[])
{
	m_graphicsCommandList->IASetVertexBuffers(startSlot, count, vbViews);
}

void GraphicsContext::SetDynamicVB(UINT slot, size_t numVertices, size_t vertexStride, const void* vbData)
{
	//set the dynamic vertex buffer
	size_t bufferSize = Mathematics::AlignUp(numVertices * vertexStride, 16);
	DynamicAlloc vertextBufferAllocated = m_cpuLinearAllocator.Allocate(bufferSize);

	memcpy(vertextBufferAllocated.m_cpuVirtualAddress, vbData, bufferSize);

	D3D12_VERTEX_BUFFER_VIEW vbView;
	vbView.BufferLocation = vertextBufferAllocated.m_gpuVirtualAddress;
	vbView.SizeInBytes = (UINT)bufferSize;
	vbView.StrideInBytes = (UINT)vertexStride;

	m_graphicsCommandList->IASetVertexBuffers(slot, 1, &vbView);
}

void GraphicsContext::SetDynamicIB(size_t indexCount, const uint16_t* IBData)
{
	size_t bufferSize = Mathematics::AlignUp(indexCount * sizeof(uint16_t), 16);
	DynamicAlloc ib = m_cpuLinearAllocator.Allocate(bufferSize);

	memcpy(ib.m_cpuVirtualAddress, IBData, bufferSize >> 4);

	D3D12_INDEX_BUFFER_VIEW IBView;
	IBView.BufferLocation = ib.m_gpuVirtualAddress;
	IBView.SizeInBytes = (UINT)(indexCount * sizeof(uint16_t));
	IBView.Format = DXGI_FORMAT_R16_UINT;

	m_graphicsCommandList->IASetIndexBuffer(&IBView);
}

void GraphicsContext::SetDynamicSRV(UINT rootIndex, size_t bufferSize, const void* bufferData)
{
	DynamicAlloc cb = m_cpuLinearAllocator.Allocate(bufferSize);
	memcpy(cb.m_cpuVirtualAddress, bufferData, Mathematics::AlignUp(bufferSize, 16) >> 4);
	m_graphicsCommandList->SetGraphicsRootShaderResourceView(rootIndex, cb.m_gpuVirtualAddress);
}

void GraphicsContext::Draw(UINT vertexCount, UINT vertexStartOffset)
{
	DrawInstanced(vertexCount, 1, vertexStartOffset, 0);
}

void GraphicsContext::DrawIndexed(UINT indexCount, UINT startIndexLocation, INT baseVertexLocation)
{
	DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
}

void GraphicsContext::DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation)
{
	FlushResourceBarrier();
	m_dynamicViewDescriptorHeap.CommitGraphicsDescriptorTablesOfRootSignature(m_graphicsCommandList);
	m_dynamicSamplerDescriptorHeap.CommitGraphicsDescriptorTablesOfRootSignature(m_graphicsCommandList);
	m_graphicsCommandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void GraphicsContext::DrawIndexedInstanced(UINT indexCountPerInstance, UINT InstanceCount, 
	UINT startIndexLocation, UINT startVertexLocation, UINT startInstanceLocation)
{
	FlushResourceBarrier();
	m_dynamicViewDescriptorHeap.CommitGraphicsDescriptorTablesOfRootSignature(m_graphicsCommandList);
	m_dynamicSamplerDescriptorHeap.CommitGraphicsDescriptorTablesOfRootSignature(m_graphicsCommandList);
	m_graphicsCommandList->DrawIndexedInstanced(indexCountPerInstance, InstanceCount,
		startIndexLocation, startVertexLocation, startInstanceLocation);
}
