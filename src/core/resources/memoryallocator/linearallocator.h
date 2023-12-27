#pragma once

#include "../gpuresource.h"
#include <vector>
#include <queue>
#include <mutex>

//constant block is multiples of 16
#define DEFAULT_ALIGN 256

enum LinearAllocationType
{
	CPU_MEMORY_ALLOCATION = 0,
	GPU_MEMORY_ALLOCATION = 1
};

enum LinearAllocatorPageSize {
	gpuAllocatorPageSize = 0x10000, //64k
	cpuAllocatorPageSize = 0x200000 //2mb
};

//The memory allocated based on Linear Page
struct DynamicAlloc
{
	DynamicAlloc(GPUResource& baseResource, size_t _offset, size_t _size) :
		m_resource(baseResource), offset(_offset), size(_size) {}

	GPUResource& m_resource;
	D3D12_GPU_VIRTUAL_ADDRESS m_gpuVirtualAddress;
	void* m_cpuVirtualAddress;
	size_t offset;
	size_t size;
};


//The linear page records the linear memory allocated 
class LinearPage : public GPUResource
{
public:
	LinearPage(ID3D12Resource* pResource, D3D12_RESOURCE_STATES usage) : GPUResource()
	{
		m_resource = pResource;
		m_usageState = usage;
		m_gpuAddress = m_resource->GetGPUVirtualAddress();
		m_resource->Map(0, nullptr, &m_cpuVirtualAddress);
	}

	~LinearPage(){}

	void Map() {
		if (m_cpuVirtualAddress == nullptr)
			m_resource->Map(0, nullptr, &m_cpuVirtualAddress);
	}

	void Unmap() {
		if (m_cpuVirtualAddress != nullptr) {
			m_resource->Unmap(0, nullptr);
			m_cpuVirtualAddress = nullptr;
		}
	}

	void* m_cpuVirtualAddress;
	D3D12_GPU_VIRTUAL_ADDRESS m_gpuVirtualAddress;
};

//Linear Page Allocation 
class LinearAllocationPageManager
{
public:
	LinearAllocationPageManager();
	LinearPage* RequestPage(void);
	LinearPage* CreateNewPage(size_t pageSize = 0);
	void DiscardPages(uint64_t fenceID, const std::vector<LinearPage*>& pages);
	void FreeLargePages(uint64_t fenceID, const std::vector <LinearPage*>& pages);
	void Destroy();

private:
	static LinearAllocationType allocationType;
	LinearAllocationType m_allocationType;
	//all the linear pages
	std::vector<std::unique_ptr<LinearPage>> m_pagesPool;
	//the linear pages waits for reuse
	std::queue<std::pair<uint64_t, LinearPage*>> m_retiredPages;
	//the linear pages ready to delete
	std::queue<std::pair<uint64_t, LinearPage*>> m_deleteQueue;
	//the linear pages available to use
	std::queue<LinearPage*> m_availablePages;
	std::mutex m_mutex;
};


//Dynamic Memory Allocator
//apply for one linear memory page, and split the memory for requests
class DynamicLinearMemoryAllocator
{
public:
	DynamicLinearMemoryAllocator(LinearAllocationType type) :
		m_curType(type), m_curPage(nullptr), m_curPageSize(0), m_curOffset(0) {
		if (type == CPU_MEMORY_ALLOCATION)
			m_curPageSize = LinearAllocatorPageSize::cpuAllocatorPageSize;
		else if (type == GPU_MEMORY_ALLOCATION)
			m_curPageSize = LinearAllocatorPageSize::gpuAllocatorPageSize;

		assert(m_curPageSize != 0);
	}

	DynamicAlloc Allocate(size_t size, size_t alignment = DEFAULT_ALIGN);
	void ClearUpPages(uint64_t fenceValue);
	static void DestroyAll() {
		g_allocator[CPU_MEMORY_ALLOCATION].Destroy();
		g_allocator[GPU_MEMORY_ALLOCATION].Destroy();
	}

private:
	DynamicAlloc AllocateLargePage(size_t sizeInBytes);

	//all dynamic linear memory allocators are based on two page managers
	static LinearAllocationPageManager g_allocator[2];


	LinearAllocationType m_curType;
	LinearPage* m_curPage; 
	size_t m_curPageSize;
	size_t m_curOffset;
	std::vector<LinearPage*> m_retiredPages;
	std::vector<LinearPage*> m_largePages; 
};



