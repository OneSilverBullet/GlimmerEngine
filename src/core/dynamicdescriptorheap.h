#pragma once

#include <mutex>
#include <vector>
#include <string>
#include <queue>
#include <d3d12.h>
#include <wrl/client.h>
#include "descriptortypes.h"
#include "rootsignature.h"

class DynamicDescriptorsManager
{
public:
	static DynamicDescriptorsManager& Instance() {
		static DynamicDescriptorsManager instance;
		return instance;
	}

	ID3D12DescriptorHeap* RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType);

	void DiscardDescriptors(
		D3D12_DESCRIPTOR_HEAP_TYPE heapType,
		uint64_t fenceValue, 
		const std::vector<ID3D12DescriptorHeap*>& usedHeaps);

private:
	DynamicDescriptorsManager(){}
	DynamicDescriptorsManager(const DynamicDescriptorsManager& copy) = delete;
	DynamicDescriptorsManager& operator=(const DynamicDescriptorsManager& v) = delete;
	uint32_t GetDescriptorHeapIdx(D3D12_DESCRIPTOR_HEAP_TYPE heapType);

private:
	std::mutex m_mutex;
	std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> m_descriptorHeapsPool[2];
	std::queue<std::pair<uint32_t, ID3D12DescriptorHeap*>> m_retiredDescriptorHeap[2];
	std::queue<ID3D12DescriptorHeap*> m_availableHeap[2];
};

//the dynamic descriptors cache structure
struct DescriptorHandlesCache
{
	DescriptorHandlesCache();


	void ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE type, const RootSignature& rootSig);

	//the max number of descriptors and descriptor tables
	static const uint32_t maxNumDescriptors = 256; 
	static const uint32_t maxNumDescriptorTables = 16;

	//the bit map of descriptor table in current root signature 
	uint32_t m_rootDescriptorTablesBitMap;
	//the non empty slot of current root signature
	uint32_t m_staleRootParamsBitMap;

	DescriptorTableEntry m_rootDescriptorTable[maxNumDescriptorTables];
	D3D12_CPU_DESCRIPTOR_HANDLE m_descriptors[maxNumDescriptors];
};


class DynamicDescriptorHeap
{







};