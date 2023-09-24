#include "commandqueue.h"
#include "headers.h"
#include <wrl.h>
#include <queue>
#include <cstdint>

using namespace Microsoft::WRL;

CommandQueue::CommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type) 
	: m_device(device), m_commandListType(type)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue)));
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(m_fenceEvent && "Failed to create fence event.");
}

CommandQueue::~CommandQueue() {
	::CloseHandle(m_fenceEvent);
}


ComPtr<ID3D12GraphicsCommandList2> CommandQueue::GetCommandList() {
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList2> commandList;

	//Create or reuse the command allocator
	if (!m_commandAllocatorQueue.empty() && IsFenceComplete(m_commandAllocatorQueue.front().fenceValue)) {
		ComPtr<ID3D12CommandAllocator> allocator = m_commandAllocatorQueue.front().commandAllocator;
		m_commandAllocatorQueue.pop();
		ThrowIfFailed(allocator->Reset());
	}
	else {
		commandAllocator = CreateCommandAllocator();
	}

	//Create or reuse the command list
	if (!m_commandListQueue.empty()) {
		commandList = m_commandListQueue.front();
		m_commandListQueue.pop();
		ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
	}
	else {
		commandList = CreateCommandList(commandAllocator);
	}

	//assign the command allocator to the private data of command list
	//build an assocation between the command allocator and the command list for retriving the allocator
	ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));
	return commandList;
}


uint64_t CommandQueue::ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2> commandList) {
	commandList->Close();
	ID3D12CommandAllocator* commandAllocator;
	UINT dataSize = sizeof(commandAllocator);
	ThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));
	
	ID3D12CommandList* const ppCommandLists[] = {
		commandList.Get()
	};

	//execute the command list
	m_commandQueue->ExecuteCommandLists(1, ppCommandLists);
	uint64_t fenceValue = Signal();

	//retrive the allocator and command list
	m_commandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator });
	m_commandListQueue.push(commandList);

	commandAllocator->Release();
	return fenceValue;
}

uint64_t CommandQueue::Signal() {
	uint64_t fenceValue = ++m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fenceValue));
	return fenceValue;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue) const {
	if (m_fence->GetCompletedValue() >= fenceValue) {
		return true;
	}
	return false;
}

void CommandQueue::WaiteForFenceValue(uint64_t fenceValue) {
	if (m_fence->GetCompletedValue() < fenceValue) {

		ThrowIfFailed(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent));
		::WaitForSingleObject(m_fenceEvent, DWORD_MAX);
	}
}

void CommandQueue::Flush() {
	UINT64 fenceValueForSignal = Signal();
	WaiteForFenceValue(fenceValueForSignal);
}

ComPtr<ID3D12CommandQueue> CommandQueue::GetCommandQueue() const {

	return m_commandQueue;
}


ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator() {
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(m_device->CreateCommandAllocator(m_commandListType, IID_PPV_ARGS(&commandAllocator)));
	return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList2> CommandQueue::CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator) {
	ComPtr<ID3D12GraphicsCommandList2> commandList;
	ThrowIfFailed(m_device->CreateCommandList(0, m_commandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
	ThrowIfFailed(commandList->Close());
	return commandList;
}

