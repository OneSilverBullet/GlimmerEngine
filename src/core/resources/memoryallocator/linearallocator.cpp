#pragma once
#include "linearallocator.h"
#include "graphicscore.h"
#include "mathematics/bitoperation.h"


LinearAllocationPageManager::LinearAllocationPageManager()
{
}

LinearPage* LinearAllocationPageManager::RequestPage(void)
{
	std::lock_guard<std::mutex> lockguard(m_mutex);

	//process the retired pages
	while (!m_retiredPages.empty() && GRAPHICS_CORE::g_commandManager.IsFenceComplete(m_retiredPages.front().first)) {
		m_availablePages.push(m_retiredPages.front().second);
		m_retiredPages.pop();
	}

	LinearPage* pagePtr = nullptr;

	//Create or reuse the linear memory page
	if (!m_availablePages.empty()) {
		pagePtr = m_availablePages.front();
		m_availablePages.pop();
	}
	else {
		pagePtr = CreateNewPage();
		m_pagesPool.emplace_back(pagePtr);
	}

	return pagePtr;
}

LinearPage* LinearAllocationPageManager::CreateNewPage(size_t pageSize)
{
	//create heap properties
	D3D12_HEAP_PROPERTIES heapProp;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 0;
	heapProp.VisibleNodeMask = 0;

	//create gpu resource description
	D3D12_RESOURCE_DESC gpuResourceDesc;
	gpuResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	gpuResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	gpuResourceDesc.Alignment = 0;
	gpuResourceDesc.Height = 1;
	gpuResourceDesc.DepthOrArraySize = 1;
	gpuResourceDesc.MipLevels = 1;
	gpuResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	gpuResourceDesc.SampleDesc.Count = 1;
	gpuResourceDesc.SampleDesc.Quality = 0;

	D3D12_RESOURCE_STATES defaultUsage;

	//for gpu, the allocated memory used for computer shader
	if (m_allocationType == GPU_MEMORY_ALLOCATION) {
		heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
		gpuResourceDesc.Width = pageSize == 0 ? gpuAllocatorPageSize : pageSize;
		gpuResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		defaultUsage = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}
	else { //for cpu, the allocated memory used for uploading data to GPU
		heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
		gpuResourceDesc.Width = pageSize == 0 ? cpuAllocatorPageSize : pageSize;
		gpuResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		defaultUsage = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	//Create the resource
	ID3D12Resource* bufferInstance;

	ThrowIfFailed(GRAPHICS_CORE::g_device->CreateCommittedResource(
		&heapProp, D3D12_HEAP_FLAG_NONE, &gpuResourceDesc, defaultUsage,
		nullptr, IID_PPV_ARGS(&bufferInstance)));
	bufferInstance->SetName(L"LinearAllocatorPage");
	return new LinearPage(bufferInstance, defaultUsage);
}

void LinearAllocationPageManager::DiscardPages(uint64_t fenceID, const std::vector<LinearPage*>& pages)
{
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	for (size_t i = 0; i < pages.size(); ++i) {
		m_retiredPages.push(std::make_pair(fenceID, pages[i]));
	}
}

void LinearAllocationPageManager::FreeLargePages(uint64_t fenceID, const std::vector<LinearPage*>& pages)
{
	std::lock_guard <std::mutex> lockGuard(m_mutex);

	while (!m_deleteQueue.empty() && GRAPHICS_CORE::g_commandManager.IsFenceComplete(m_deleteQueue.front().first))
	{
		delete m_deleteQueue.front().second;
		m_deleteQueue.pop();
	}

	//To delete a memory, we should ummap the current gpu resource
	for (size_t i = 0; i < pages.size(); ++i) {
		pages[i]->Unmap();
		m_deleteQueue.push(std::make_pair(fenceID, pages[i]));
	}
}

//the pages pool contains unique ptr of allocation pages
//which means we can release the resources safely
void LinearAllocationPageManager::Destroy() {
	if (!m_pagesPool.empty()) {
		m_pagesPool.clear();
	}
}

LinearAllocationPageManager DynamicLinearMemoryAllocator::g_allocator[2];

DynamicAlloc DynamicLinearMemoryAllocator::Allocate(size_t size, size_t alignment)
{
	//from 100000000 to 011111111
	const size_t alignmentMask = alignment - 1;
	//calculate a aligned page size
	const size_t alignPageSize = Mathematics::AlignUpWithMask<size_t>(size, alignment);

	//apply for a large page if the request size is bigger than current page size 
	if (alignPageSize > m_curPageSize)
		return AllocateLargePage(alignPageSize);

	//align the current offset
	m_curOffset = Mathematics::AlignUpWithMask<size_t>(m_curOffset, alignmentMask);

	//the memory of current page is not enough for the request
	if (m_curOffset + alignPageSize > m_curPageSize) {
		m_retiredPages.push_back(m_curPage);
		m_curPage = nullptr;
	}

	if (m_curPage == nullptr) {
		m_curPage = g_allocator[m_curType].CreateNewPage(); //apply for default memory page
		m_curOffset = 0;
	}

	//Just apply a slice of current memory page
	DynamicAlloc dynamicAlloc(*m_curPage, m_curOffset, alignPageSize);
	dynamicAlloc.m_cpuVirtualAddress = (uint8_t*)m_curPage->m_cpuVirtualAddress + m_curOffset;
	dynamicAlloc.m_gpuVirtualAddress = m_curPage->m_gpuVirtualAddress + m_curOffset;

	m_curOffset += alignPageSize;

	return dynamicAlloc;
}

void DynamicLinearMemoryAllocator::ClearUpPages(uint64_t fenceValue)
{
	if (m_curPage == nullptr)
		return;

	m_retiredPages.push_back(m_curPage);
	m_curPage = nullptr;
	m_curOffset = 0;

	g_allocator[m_curType].DiscardPages(fenceValue, m_retiredPages);
	m_retiredPages.clear();

	g_allocator[m_curType].FreeLargePages(fenceValue, m_largePages);
	m_largePages.clear();
}

DynamicAlloc DynamicLinearMemoryAllocator::AllocateLargePage(size_t sizeInBytes)
{
	LinearPage* largeMemPage = g_allocator[m_curType].CreateNewPage(sizeInBytes);
	m_largePages.push_back(largeMemPage);

	DynamicAlloc largePage(*largeMemPage, 0, sizeInBytes);
	largePage.m_cpuVirtualAddress = largeMemPage->m_cpuVirtualAddress;
	largePage.m_gpuVirtualAddress = largeMemPage->m_gpuVirtualAddress;

	return largePage;
}
