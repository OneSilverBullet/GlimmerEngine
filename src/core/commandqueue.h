#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <queue>
#include <cstdint>
#include "commandallocatorpool.h"

using namespace Microsoft::WRL;

/*
* The Encapsualted CommandQueue
*/
class CommandQueue
{
public:

	CommandQueue(D3D12_COMMAND_LIST_TYPE type);
	virtual ~CommandQueue();

	void Initialize(ID3D12Device* device);
	void Release();

	inline bool IsReady()const { return m_pFence != nullptr; }


	uint64_t ExecuteCommandList(ID3D12GraphicsCommandList2* commandList);

	uint64_t IncrementFence();
	bool IsFenceComplete(uint64_t fenceValue);
	void StallForFence(uint64_t fenceValue);
	void StallForProducer(CommandQueue& producer);
	void WaitForFence(uint64_t fenceValue);
	void WaitForIdle(){ WaitForFence(IncrementFence());}

	ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueuePtr; }
	uint64_t GetNextFenceValue() const { return m_nextFenceValue; }

protected:
	ID3D12CommandAllocator* RequestAllocator();
	void DiscardCommandAllocator(uint64_t fenceValueForReset, ID3D12CommandAllocator* allocatorForDiscard);

private:
	ID3D12CommandQueue* m_commandQueuePtr;

	const D3D12_COMMAND_LIST_TYPE m_commandListType;

	std::mutex m_fenceMutex;
	std::mutex m_eventMutex;

	ID3D12Fence* m_pFence;
	uint64_t m_lastCompletedFenceValue;
	uint64_t m_nextFenceValue;
	HANDLE m_fenceEvent;
	CommandAllocatorPool m_commandAllocatorPool;
};






