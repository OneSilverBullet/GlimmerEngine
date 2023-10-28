#include "commandmanager.h"
#include "graphicscore.h"


namespace GRAPHICS_CORE
{
	extern CommandManager g_commandManager;
}


CommandManager::CommandManager():
	m_device(nullptr),
	m_graphicsQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
	m_computeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
	m_copyQueue(D3D12_COMMAND_LIST_TYPE_COPY)
{
}

CommandManager::~CommandManager()
{
	Release();
}

void CommandManager::Initialize(ID3D12Device* pDevice)
{
	m_device = pDevice;
	m_graphicsQueue.Initialize(pDevice);
	m_computeQueue.Initialize(pDevice);
	m_copyQueue.Initialize(pDevice);
}

void CommandManager::Release()
{
	m_graphicsQueue.Release();
	m_computeQueue.Release();
	m_copyQueue.Release();
}

void CommandManager::ReallocateCommandAllocator(uint64_t fenceValue, ID3D12CommandAllocator* pAllocator) {
	D3D12_COMMAND_LIST_TYPE commandListType = GetCommandListTypeFromFenceValue(fenceValue);
	GetQueue(commandListType).DiscardCommandAllocator(fenceValue, pAllocator);
}

void CommandManager::CreateNewCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12GraphicsCommandList** ppCommandList, ID3D12CommandAllocator** ppAllocator)
{
	//there is no need to create command list again
	assert((*ppCommandList) == nullptr);
	RequestCommandAllocactor(type, ppAllocator);
	ThrowIfFailed(m_device->CreateCommandList(1, type, *ppAllocator, nullptr, IID_PPV_ARGS(ppCommandList)));
	(*ppCommandList)->SetName(L"CommandList");
}

void CommandManager::ResetCommandList(
	D3D12_COMMAND_LIST_TYPE type,
	ID3D12GraphicsCommandList** ppCommandList,
	ID3D12CommandAllocator** ppAllocator) 
{
	assert((*ppCommandList) != nullptr);
	RequestCommandAllocactor(type, ppAllocator);
	(*ppCommandList)->Reset((*ppAllocator), nullptr);
}

void CommandManager::RequestCommandAllocactor(D3D12_COMMAND_LIST_TYPE type,
	ID3D12CommandAllocator** ppAllocator) {
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		*ppAllocator = m_graphicsQueue.RequestAllocator();
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		*ppAllocator = m_computeQueue.RequestAllocator();
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		*ppAllocator = m_copyQueue.RequestAllocator();
		break;
	default:
		break;
	}
	assert((*ppAllocator) == nullptr);
}

bool CommandManager::IsFenceComplete(uint64_t fenceValue)
{
	return GetQueue((D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56)).IsFenceComplete(fenceValue);
}

void CommandManager::WaitForFence(uint64_t fenceValue)
{
	GetQueue((D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56)).WaitForFence(fenceValue);
}

uint64_t CommandManager::CreateFenceValue(D3D12_COMMAND_LIST_TYPE type, uint64_t origin)
{
	return ((type << 56) | origin);
}

D3D12_COMMAND_LIST_TYPE CommandManager::GetCommandListTypeFromFenceValue(uint64_t fenceValue) {
	return (D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56);
}

