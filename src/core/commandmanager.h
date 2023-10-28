#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <queue>
#include <cstdint>
#include "commandqueue.h"

using namespace Microsoft::WRL;


class CommandManager
{
public:
	CommandManager();
	~CommandManager();

	void Initialize(ID3D12Device* pDevice);
	void Release();

	CommandQueue& GetDirectQueue() { return m_graphicsQueue; }
	CommandQueue& GetComputeQueue() { return m_computeQueue; }
	CommandQueue& GetCopyQueue() { return m_copyQueue; }	

	CommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE type) {
		switch (type)
		{	
		case D3D12_COMMAND_LIST_TYPE_COMPUTE:
			return m_computeQueue;
		case D3D12_COMMAND_LIST_TYPE_COPY:
			return m_copyQueue;
		default:
			return m_graphicsQueue;
		}
	}

	ID3D12CommandQueue* GetCommandQueue(){ return m_graphicsQueue.GetCommandQueue(); }

	void CreateNewCommandList(
		D3D12_COMMAND_LIST_TYPE type,
		ID3D12GraphicsCommandList** ppCommandList,
		ID3D12CommandAllocator** ppAllocator);

	void ResetCommandList(
		D3D12_COMMAND_LIST_TYPE type,
		ID3D12GraphicsCommandList** ppCommandList,
		ID3D12CommandAllocator** ppAllocator);


	void ReallocateCommandAllocator(uint64_t fenceValue, ID3D12CommandAllocator* pAllocator);

	//create a new fence value according to the queue type
	uint64_t CreateFenceValue(D3D12_COMMAND_LIST_TYPE type, uint64_t origin);

	//get the fence value's command list type
	D3D12_COMMAND_LIST_TYPE GetCommandListTypeFromFenceValue(uint64_t fenceValue);

	//waits for a fence value to be reached
	bool IsFenceComplete(uint64_t fenceValue);

	//waits for a fence value to be reached
	void WaitForFence(uint64_t fenceValue);

	//waits for all queues to finish
	void Flush() { 
		m_graphicsQueue.Flush(); 
		m_computeQueue.Flush();
		m_copyQueue.Flush();
	}

protected:
	void RequestCommandAllocactor(D3D12_COMMAND_LIST_TYPE type,
		ID3D12CommandAllocator** ppAllocator);


private:
	ID3D12Device* m_device;

	CommandQueue m_graphicsQueue;
	CommandQueue m_computeQueue;
	CommandQueue m_copyQueue;
};


