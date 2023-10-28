#include "commandqueue.h"
#include "headers.h"
#include <wrl.h>
#include <queue>
#include <cstdint>

using namespace Microsoft::WRL;

CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type)
	: m_commandListType(type),
	  m_commandQueuePtr(nullptr),
	  m_pFence(nullptr),
	  m_nextFenceValue((uint64_t)type << 56 | 1),
	  m_lastCompletedFenceValue((uint64_t)type << 56),
	  m_commandAllocatorPool(type)
{
}

CommandQueue::~CommandQueue() {
	::CloseHandle(m_fenceEvent);
}

void CommandQueue::Initialize(ID3D12Device* device) {
	assert(device != nullptr);
	assert(!IsReady());
	assert(m_commandAllocatorPool.GetPoolSize() == 0);

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = m_commandListType;
	queueDesc.NodeMask = 1;
	
	device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pFence));
	m_commandQueuePtr->SetName(L"CommandListManager::m_commandQueue");

	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
	m_pFence->SetName(L"CommandListManager::m_pFence");
	m_pFence->Signal((uint64_t)m_commandListType << 56);

	m_fenceEvent = ::CreateEvent(nullptr, false, false, nullptr);
	assert(m_fenceEvent != INVALID_HANDLE_VALUE);

	m_commandAllocatorPool.Initialize(device);
	assert(IsReady());
}

void CommandQueue::Release() {
	if (m_commandQueuePtr != nullptr)
		return;

	m_commandAllocatorPool.Release();

	CloseHandle(m_fenceEvent);

	m_pFence->Release();
	m_pFence = nullptr;

	m_commandQueuePtr->Release();
	m_commandQueuePtr = nullptr;
}

uint64_t CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList2* commandList)
{
	std::lock_guard<std::mutex> lock(m_fenceMutex);
	ThrowIfFailed(commandList->Close());
	//execute command list
	m_commandQueuePtr->ExecuteCommandLists(1, (ID3D12CommandList* const*)&commandList);
	//create marke
	m_commandQueuePtr->Signal(m_pFence, m_nextFenceValue);
	//increase the fence value
	return m_nextFenceValue++;
}

uint64_t CommandQueue::IncrementFence()
{
	std::lock_guard<std::mutex> lock(m_fenceMutex);
	m_commandQueuePtr->Signal(m_pFence, m_nextFenceValue);
	return m_nextFenceValue++;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
	//If current fence not complete, update last compeleted fence
	if (fenceValue > m_lastCompletedFenceValue)
	{
		m_lastCompletedFenceValue = std::max(m_lastCompletedFenceValue, m_pFence->GetCompletedValue());
	}
	return fenceValue <= m_lastCompletedFenceValue;
}

void CommandQueue::WaitForFence(uint64_t fenceValue)
{
	if (IsFenceComplete(fenceValue))
		return;

	{
		std::lock_guard<std::mutex> lock(m_eventMutex);

		m_pFence->SetEventOnCompletion(fenceValue, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
		//update last completed fence
		m_lastCompletedFenceValue = fenceValue;
	}
}

void CommandQueue::StallForFence(uint64_t fenceValue)
{
	//Todo: stall for fence
}

void CommandQueue::StallForProducer(CommandQueue& producer)
{
	//Todo: stall for producer
}

ID3D12CommandAllocator* CommandQueue::RequestAllocator() {
	uint64_t fenceValueForReset = m_pFence->GetCompletedValue();
	return m_commandAllocatorPool.RequestAllocator(fenceValueForReset);
}

void CommandQueue::DiscardCommandAllocator(uint64_t fenceValueForReset, 
	ID3D12CommandAllocator* allocatorForDiscard) {
	m_commandAllocatorPool.DiscardAllocator(fenceValueForReset, allocatorForDiscard);
}
