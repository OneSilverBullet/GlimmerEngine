#pragma once

#include <mutex>
#include <vector>
#include <string>
#include <d3d12.h>
#include "dynamicdescriptorheap.h"
#include "graphicscore.h"

const uint32_t g_maxDescriptorsNumPerHeap = 1024;

enum DynamicDescriptorHeapTypes
{
	CBV_SRV_UAV_RTV_DSV = 0,
	SAMPLER = 1
};

/*
* DynamicDescriptorsManager
*/
ID3D12DescriptorHeap* DynamicDescriptorsManager::RequestDescriptorHeap(
	D3D12_DESCRIPTOR_HEAP_TYPE heapType) {
	
	uint32_t heapIdx = GetDescriptorHeapIdx(heapType);
	std::lock_guard<std::mutex> lockGuard(m_mutex);

	//update the descriptor heaps
	while (!m_retiredDescriptorHeap[heapIdx].empty() &&
		GRAPHICS_CORE::g_commandManager.IsFenceComplete(m_retiredDescriptorHeap[heapIdx].front().first))
	{
		m_availableHeap[heapIdx].push(m_retiredDescriptorHeap[heapIdx].front().second);
		m_retiredDescriptorHeap[heapIdx].pop();
	}
	
	//if there is available heap, we return the heap pointer
	if (!m_availableHeap[heapIdx].empty())
	{
		ID3D12DescriptorHeap* heapPtr = m_availableHeap[heapIdx].front();
		m_availableHeap[heapIdx].pop();
		return heapPtr;
	}
	else { //if there is no available heap, we create a new descriptor heaps
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = heapType;
		heapDesc.NumDescriptors = g_maxDescriptorsNumPerHeap;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NodeMask = 0;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heapPtr;
		ThrowIfFailed(GRAPHICS_CORE::g_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heapPtr)));
		return heapPtr.Get();
	}
}

void DynamicDescriptorsManager::DiscardDescriptors(
	D3D12_DESCRIPTOR_HEAP_TYPE heapType,
	uint64_t fenceValue,
	const std::vector<ID3D12DescriptorHeap*>& usedHeaps) {
	uint32_t heapIdx = GetDescriptorHeapIdx(heapType);
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	for (auto iter = usedHeaps.begin(); iter != usedHeaps.end(); iter++)
		m_retiredDescriptorHeap[heapIdx].push(std::make_pair(fenceValue, *iter));
}

//process descriptors heap
uint32_t DynamicDescriptorsManager::GetDescriptorHeapIdx(D3D12_DESCRIPTOR_HEAP_TYPE
	heapType) {
	uint32_t idx;
	if (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
		idx = DynamicDescriptorHeapTypes::SAMPLER;
	else
		idx = DynamicDescriptorHeapTypes::CBV_SRV_UAV_RTV_DSV;
	return idx;
}


DescriptorHandlesCache::DescriptorHandlesCache() {
	ReleaseCaches();
}

void DescriptorHandlesCache::CommitDescriptorHandleToDescriptorHeap(
	D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptorsSize,
	DescriptorHandle dstHandleStart, ID3D12GraphicsCommandList* cmdList,
	void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)) {

	//Extract the dynamic descriptor heaps properties for efficiency
	uint32_t descriptorTableSize[maxNumDescriptorTables];
	uint32_t rootIndexDoc[maxNumDescriptorTables];
	uint32_t assignedDescriptorTableCount = 0;
	unsigned long descriptorsSpaceSize = 0;
	unsigned long rootIndex = 0;

	unsigned long assignedDescriptorTablesBitMap = m_assignedRootParamsBitMap;
	while (_BitScanForward(&rootIndex, assignedDescriptorTablesBitMap)) {
		rootIndexDoc[assignedDescriptorTableCount] = rootIndex;
		assignedDescriptorTablesBitMap ^= (1 << rootIndex);
		//extract the descriptors space in current descriptor table
		unsigned long descriptorsNumInDescriptorTable;
		_BitScanReverse(&descriptorsNumInDescriptorTable, assignedDescriptorTablesBitMap);

		descriptorsSpaceSize += descriptorsNumInDescriptorTable + 1;
		descriptorTableSize[assignedDescriptorTableCount] = descriptorsNumInDescriptorTable + 1;
		assignedDescriptorTableCount++;
	}

	//
	static const uint32_t maxNumCopyDescriptorsNum = 16;

	uint32_t destDescriptorRangeIndex = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStart[maxNumCopyDescriptorsNum];
	uint32_t destDescriptorRangeDescriptorsSize[maxNumCopyDescriptorsNum];

	uint32_t srcDescriptorRangeIndex = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE pSrcDescriptorRangeStart[maxNumCopyDescriptorsNum];
	uint32_t srcDescriptorRangeDescriptorsSize[maxNumCopyDescriptorsNum];
	//loop each descriptor table
	for (uint32_t i = 0; i < assignedDescriptorTableCount; ++i) {
		rootIndex = rootIndexDoc[i];
		(cmdList->*SetFunc)(rootIndex, dstHandleStart);
		
		//the source descriptor handle
		DescriptorTableEntry& descriptorTableCachedInfo = m_rootDescriptorTable[rootIndex];
		D3D12_CPU_DESCRIPTOR_HANDLE* srcHandles = descriptorTableCachedInfo.tableStart;
		unsigned long assignedDescriptorBitMap = descriptorTableCachedInfo.assignedHandlesBitMap;
		//the destination descriptor handle
		D3D12_CPU_DESCRIPTOR_HANDLE dstHandles = dstHandleStart;
		dstHandleStart += descriptorTableSize[i] * descriptorsSize;

		//copy the source descriptors in descriptor table to the descriptor heap and recover the descriptor layout
		unsigned long skipCount;
		while (_BitScanForward(&skipCount, assignedDescriptorBitMap))
		{
			//skip the first several empty descriptor slots
			assignedDescriptorBitMap >>= skipCount;
			srcHandles += skipCount;
			dstHandles.ptr += skipCount * descriptorsSize;

			//gain the non-empty descriptor slots number in current filled block
			unsigned long descriptorCount;
			_BitScanForward(&descriptorCount, ~assignedDescriptorBitMap);
			assignedDescriptorBitMap >>= descriptorCount;

			//if current descriptors block is extend the max copy number
			if (srcDescriptorRangeIndex + descriptorCount > maxNumCopyDescriptorsNum)
			{
				//copy the data directly, current loop's data will be copied in the next loop
				GRAPHICS_CORE::g_device->CopyDescriptors(
					destDescriptorRangeIndex, pDestDescriptorRangeStart, destDescriptorRangeDescriptorsSize,
					srcDescriptorRangeIndex, pSrcDescriptorRangeStart, srcDescriptorRangeDescriptorsSize, type
				);

				srcDescriptorRangeIndex = 0;
				destDescriptorRangeIndex = 0;
			}

			//initial the descriptor range infomation of destination descriptor block
			pDestDescriptorRangeStart[destDescriptorRangeIndex] = dstHandles;
			destDescriptorRangeDescriptorsSize[destDescriptorRangeIndex] = descriptorCount;
			destDescriptorRangeIndex++;

			//initial the descriptor range information of source descriptor block and load the source data
			for (int srcDescriptorIndex = 0; srcDescriptorIndex < descriptorCount; srcDescriptorIndex++)
			{
				pSrcDescriptorRangeStart[srcDescriptorRangeIndex] = srcHandles[srcDescriptorIndex];
				srcDescriptorRangeDescriptorsSize[srcDescriptorRangeIndex] = 1;
				srcDescriptorRangeIndex++;
			}

			srcHandles += descriptorCount;
			dstHandles.ptr += descriptorCount * descriptorsSize;
		}
	}

	GRAPHICS_CORE::g_device->CopyDescriptors(
		destDescriptorRangeIndex, pDestDescriptorRangeStart, destDescriptorRangeDescriptorsSize,
		srcDescriptorRangeIndex, pSrcDescriptorRangeStart, srcDescriptorRangeDescriptorsSize, type
	);
}

uint32_t DescriptorHandlesCache::ComputeAssignedDescriptorsSize() {
	//calculate the descriptors size need for the descriptor heap.
	uint32_t assignedSize = 0;
	unsigned long assignedDescriptorBitMap = m_assignedRootParamsBitMap;
	unsigned long rootIndex;
	while (_BitScanForward(&rootIndex, assignedDescriptorBitMap)) {
		assignedDescriptorBitMap ^= (1 << rootIndex);
		unsigned long descriptorsSize;
		assert(TRUE == _BitScanReverse(&descriptorsSize, m_rootDescriptorTable[rootIndex].assignedHandlesBitMap));
		assignedSize += descriptorsSize + 1;
	}
	return assignedSize;
}

void DescriptorHandlesCache::StoreDescriptorsCPUHandles(UINT rootIndex, UINT offset, UINT handlesCount, const D3D12_CPU_DESCRIPTOR_HANDLE descriptorsHandleList[]) {
	DescriptorTableEntry& curTableEntry = m_rootDescriptorTable[rootIndex];
	D3D12_CPU_DESCRIPTOR_HANDLE* copyDest = curTableEntry.tableStart + offset;
	for (int i = 0; i < handlesCount; ++i)
		copyDest[i] = descriptorsHandleList[i];
	//record the assigned cpu handles bit map
	curTableEntry.assignedHandlesBitMap |= ((1 << handlesCount) - 1) << offset;
	//indicate the root index descriptors table is valid
	m_assignedRootParamsBitMap |= (1 << rootIndex);

}

//analysis the descriptor tables information in root signature
void DescriptorHandlesCache::ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE type, 
	const RootSignature& rootSig) {

	uint32_t currentOffset = 0;
	m_assignedRootParamsBitMap = 0;
	if (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
		m_rootDescriptorTablesBitMap = rootSig.m_samplerBitMap;
	else
		m_rootDescriptorTablesBitMap = rootSig.m_descriptorTableBitMap;

	unsigned long tmpBitMap = m_rootDescriptorTablesBitMap;
	unsigned long curDescriptorTableIndex = 0;
	while (_BitScanForward(&curDescriptorTableIndex, tmpBitMap))
	{
		tmpBitMap ^= (1 << curDescriptorTableIndex);

		uint32_t curDescriptorTableSize = rootSig.m_descriptorTableSize[curDescriptorTableIndex];
		assert(curDescriptorTableSize > 0);

		//record the descriptors table information and allocate the descriptor memory block
		DescriptorTableEntry& tableEntry = m_rootDescriptorTable[curDescriptorTableIndex];
		tableEntry.tableSize = curDescriptorTableSize;
		tableEntry.tableStart = m_descriptors + currentOffset;
		tableEntry.assignedHandlesBitMap = 0;
		
		currentOffset += curDescriptorTableSize;
	}
	m_cachedDescriptorsNum += currentOffset;
	assert(m_cachedDescriptorsNum < maxNumDescriptors);
}

void DescriptorHandlesCache::ReleaseCaches() {
	//lazy release: we just modify the data flag 
	m_rootDescriptorTablesBitMap = 0;
	m_assignedRootParamsBitMap = 0;
	m_cachedDescriptorsNum = 0;
}

DynamicDescriptorHeap::DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
	: m_descriptorHeapType(heapType)
{
	m_curDescriptorHeap = nullptr;
	m_currentOffset = 0;
	m_descriptorSize = GRAPHICS_CORE::g_device->GetDescriptorHandleIncrementSize(heapType);
}

DynamicDescriptorHeap::~DynamicDescriptorHeap() {

}

void DynamicDescriptorHeap::CleanupUsedHeap(uint64_t fence) {
	RetireCurrentHeap();
	RetireUsedHeaps(fence);
	m_graphicsDescriptorsCache.ReleaseCaches();
	m_computeDescriptorsCache.ReleaseCaches();
}

void DynamicDescriptorHeap::SetGraphicsDescriptorHandles(uint32_t rootIndex, uint32_t offset,
	uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE handles[]) {
	m_graphicsDescriptorsCache.StoreDescriptorsCPUHandles(rootIndex, offset, numHandles, handles);
}

void DynamicDescriptorHeap::SetComputeDescriptorHandles(uint32_t rootIndex, uint32_t offset,
	uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE handles[]) {
	m_computeDescriptorsCache.StoreDescriptorsCPUHandles(rootIndex, offset, numHandles, handles);
}

void DynamicDescriptorHeap::ParseGraphicsRootSignature(const RootSignature& rootSignature) {
	m_graphicsDescriptorsCache.ParseRootSignature(m_descriptorHeapType, rootSignature);
}

void DynamicDescriptorHeap::ParseComputeRootSignature(const RootSignature& rootSignature) {
	m_computeDescriptorsCache.ParseRootSignature(m_descriptorHeapType, rootSignature);
}

void DynamicDescriptorHeap::CommittedDescriptorTables(DescriptorHandlesCache& handleCache, ID3D12GraphicsCommandList* cmdList,
	void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE)) {
	//calculate the descriptors size in dyanmic descriptors caches 
	uint32_t needSize = handleCache.ComputeAssignedDescriptorsSize();
	if (!HasFreeSpace(needSize)) {
		RetireCurrentHeap();
	}

	//TODO:Set Descriptor Heaps to Graphics Context

	handleCache.CommitDescriptorHandleToDescriptorHeap(m_descriptorHeapType, m_descriptorSize,
		Allocate(needSize), cmdList, SetFunc);
}

void DynamicDescriptorHeap::CommitGraphicsDescriptorTablesOfRootSignature(ID3D12GraphicsCommandList* graphicsList) {
	if (m_graphicsDescriptorsCache.m_assignedRootParamsBitMap != 0) {
		CommittedDescriptorTables(m_graphicsDescriptorsCache, graphicsList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
	}
}

void DynamicDescriptorHeap::CommitComputeDescriptorTablesOfRootSignature(ID3D12GraphicsCommandList* computeList) {
	if (m_computeDescriptorsCache.m_assignedRootParamsBitMap != 0) {
		CommittedDescriptorTables(m_computeDescriptorsCache, computeList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
	}
}

DescriptorHandle DynamicDescriptorHeap::Allocate(UINT count) {
	DescriptorHandle ret = m_firstDescriptorHandle + m_currentOffset * m_descriptorSize;
	m_currentOffset += count;
	return ret;
}

bool DynamicDescriptorHeap::HasFreeSpace(UINT count) {
	return (m_curDescriptorHeap != nullptr) && (m_currentOffset + count <= kNumDescriptorsPerHeap);
}

ID3D12DescriptorHeap* DynamicDescriptorHeap::GetHeapPointer() {
	if (m_curDescriptorHeap == nullptr) {
		assert(m_currentOffset == 0);
		m_curDescriptorHeap = DynamicDescriptorsManager::Instance().RequestDescriptorHeap(m_descriptorHeapType);
		m_firstDescriptorHandle = DescriptorHandle(
			m_curDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			m_curDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	}
	return m_curDescriptorHeap;
}

void DynamicDescriptorHeap::RetireCurrentHeap() {
	//never retire empty heap
	if (m_currentOffset == 0) {
		assert(m_curDescriptorHeap == nullptr);
		return;
	}
	assert(m_curDescriptorHeap != nullptr);
	m_retiredDescriptorHeaps.push_back(m_curDescriptorHeap);
	//refresh the descriptor heap
	m_curDescriptorHeap = nullptr;
	m_currentOffset = 0;
}

void DynamicDescriptorHeap::RetireUsedHeaps(uint64_t fenceValue) {
	//discard all the retired descriptor heaps
	DynamicDescriptorsManager::Instance().DiscardDescriptors(m_descriptorHeapType, fenceValue, m_retiredDescriptorHeaps);
	m_retiredDescriptorHeaps.clear();
}
