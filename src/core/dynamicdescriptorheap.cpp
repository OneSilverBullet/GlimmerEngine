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


