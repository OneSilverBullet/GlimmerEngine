#pragma once

#include <mutex>
#include <vector>
#include <string>
#include <queue>
#include <d3d12.h>
#include <wrl/client.h>
#include "descriptortypes.h"
#include "rootsignature.h"
#include "context.h"

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

//the dynamic descriptors cache structure only consider about the descriptors tables
//the other descriptor we can bind directly to the commandlist.
struct DescriptorHandlesCache
{
	DescriptorHandlesCache();

	//compute the descriptors size need in a descriptor heap
	uint32_t ComputeAssignedDescriptorsSize();
	//store the descriptors handle to current cache
	void StoreDescriptorsCPUHandles(UINT rootIndex, UINT offset, UINT handlesCount, const D3D12_CPU_DESCRIPTOR_HANDLE descriptorsHandleList[]);
	//copy the descriptors handles in current cache to a descriptor heaps
	void CommitDescriptorHandleToDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptorsSize,
		DescriptorHandle dstHandleStart, ID3D12GraphicsCommandList* cmdList,
		void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));
	//analysis the root signature and only consider the descriptors tables
	void ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE type, const RootSignature& rootSig);
	//clear current cache
	void ReleaseCaches();

	//the max number of descriptors and descriptor tables
	static const uint32_t maxNumDescriptors = 256; 
	static const uint32_t maxNumDescriptorTables = 16;

	//the bit map of descriptor table in current root signature 
	uint32_t m_rootDescriptorTablesBitMap;
	//the non empty slot of current root signature
	uint32_t m_assignedRootParamsBitMap;
	//record the information of descriptor table in root signature
	DescriptorTableEntry m_rootDescriptorTable[maxNumDescriptorTables];
	//store the descriptor of descriptors table in a linear way
	D3D12_CPU_DESCRIPTOR_HANDLE m_descriptors[maxNumDescriptors];
	//the descriptor number
	uint32_t m_cachedDescriptorsNum;
};


class DynamicDescriptorHeap
{
public: 
	DynamicDescriptorHeap(Context& owningContext, D3D12_DESCRIPTOR_HEAP_TYPE heapType);
	~DynamicDescriptorHeap();

	void CleanupUsedHeap(uint64_t fence);
	//store the resource handles to current descriptors cache
	void SetGraphicsDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE handles[]);
	void SetComputeDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE handles[]);
	//analysis the root signature and record the descriptor tables information 
	void ParseGraphicsRootSignature(const RootSignature& rootSignature);
	void ParseComputeRootSignature(const RootSignature& rootSignature);
	//commit the descriptor tables to the command LIST
	void CommitGraphicsDescriptorTablesOfRootSignature(ID3D12GraphicsCommandList* graphicsList);
	void CommitComputeDescriptorTablesOfRootSignature(ID3D12GraphicsCommandList* computeList);


private:
	void CommittedDescriptorTables(DescriptorHandlesCache& handleCache, ID3D12GraphicsCommandList* cmdList,
		void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));
	DescriptorHandle Allocate(UINT count);
	bool HasFreeSpace(UINT count);
	ID3D12DescriptorHeap* GetHeapPointer();
	void RetireCurrentHeap();
	void RetireUsedHeaps(uint64_t fenceValue);

private: 
	Context& m_owningContext;
	static const uint32_t kNumDescriptorsPerHeap = 1024;
	DescriptorHandlesCache m_graphicsDescriptorsCache;
	DescriptorHandlesCache m_computeDescriptorsCache;

	ID3D12DescriptorHeap* m_curDescriptorHeap;
	std::vector<ID3D12DescriptorHeap*> m_retiredDescriptorHeaps;

	uint32_t m_descriptorSize;
	uint32_t m_currentOffset;
	DescriptorHandle m_firstDescriptorHandle;
	D3D12_DESCRIPTOR_HEAP_TYPE m_descriptorHeapType;
};