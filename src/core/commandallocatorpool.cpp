#include "commandallocatorpool.h"

CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type) :
	m_type(type), m_device(nullptr)
{
}

CommandAllocatorPool::~CommandAllocatorPool()
{
	Release();
}

void CommandAllocatorPool::Initialize(ID3D12Device* device)
{
	m_device = device;
}

void CommandAllocatorPool::Release()
{
	for (size_t i = 0; i < GetPoolSize(); ++i) {
		m_commandAllocatorPool[i]->Release();
	}
	m_commandAllocatorPool.clear();
}

ID3D12CommandAllocator* CommandAllocatorPool::RequestAllocator(uint64_t fenceValue)
{
	std::lock_guard<std::mutex> lockGuard(m_allocatorMutex);
	ID3D12CommandAllocator* returnAllocator = nullptr;
	if (!m_readyAllocators.empty()) {
		std::pair<uint64_t, ID3D12CommandAllocator*> allocatorItem = m_readyAllocators.front();
		//check the allocator is ready to be reused
		if (allocatorItem.first < fenceValue) {
			ID3D12CommandAllocator* allocator = allocatorItem.second;
			ThrowIfFailed(allocator->Reset());
			m_readyAllocators.pop();
			return allocator;
		}
	}

	if (returnAllocator == nullptr) {
		ThrowIfFailed(m_device->CreateCommandAllocator(m_type, IID_PPV_ARGS(&returnAllocator)));
		wchar_t allocatorName[32];
		swprintf_s(allocatorName, L"CommandAllocator %zu", m_commandAllocatorPool.size());
		returnAllocator->SetName(allocatorName);
		m_commandAllocatorPool.push_back(returnAllocator);
	}

	return returnAllocator;
}

void CommandAllocatorPool::DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator)
{
	std::lock_guard<std::mutex> lockGuard(m_allocatorMutex);
	m_readyAllocators.push(std::make_pair(fenceValue, allocator)); //the allocator is in m_commandAllocatorPool
}
