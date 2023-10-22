#include <vector>
#include <queue>
#include <mutex>
#include "headers.h"

//The command allocator is a reset every frame
//Resue the command allocator after the GPU has finished executing the commands

class CommandAllocatorPool
{
public:
	CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type);
	~CommandAllocatorPool();

	void Initialize(ID3D12Device* device);
	void Release();

	ID3D12CommandAllocator* RequestAllocator(uint64_t fenceValue);
	void DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator);

	UINT GetPoolSize()const { return (UINT)m_commandAllocatorPool.size(); }

private:
	//different command allocator pool for different type command list
	const D3D12_COMMAND_LIST_TYPE m_type; 
	ID3D12Device* m_device; 
	std::vector<ID3D12CommandAllocator*> m_commandAllocatorPool;
	std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_readyAllocators;
	std::mutex m_allocatorMutex;
};