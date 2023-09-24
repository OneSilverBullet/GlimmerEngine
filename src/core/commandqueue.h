#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <queue>
#include <cstdint>

using namespace Microsoft::WRL;

/*
* The Encapsualted CommandQueue
*/

class CommandQueue
{
public:

	CommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	virtual ~CommandQueue();

	// Get an available command list from the command queue.
	ComPtr<ID3D12GraphicsCommandList2> GetCommandList();

	//Execute a command list and recturn a fence value when the command list has finished executing.
	uint64_t ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2> commandList);

	uint64_t Signal();
	bool IsFenceComplete(uint64_t fenceValue) const;
	void WaiteForFenceValue(uint64_t fenceValue);
	void Flush();

	ComPtr<ID3D12CommandQueue> GetCommandQueue() const;

protected:
	ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
	ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator);

private:
	struct CommandAllocatorEntry
	{
		uint64_t fenceValue;
		ComPtr<ID3D12CommandAllocator> commandAllocator;
	};

	using  CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
	using  CommandListQueue = std::queue<ComPtr<ID3D12GraphicsCommandList2>>;

	D3D12_COMMAND_LIST_TYPE m_commandListType;
	ComPtr<ID3D12Device2> m_device;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12Fence> m_fence;
	HANDLE m_fenceEvent;
	uint64_t m_fenceValue;

	CommandAllocatorQueue m_commandAllocatorQueue;
	CommandListQueue m_commandListQueue;
};
