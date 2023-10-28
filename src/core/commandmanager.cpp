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

void CommandManager::CreateNewCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12GraphicsCommandList** ppCommandList, ID3D12CommandAllocator** ppAllocator)
{
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
	//if there is other command list type, shutdown here
	assert(*ppAllocator != nullptr);
	ThrowIfFailed(m_device->CreateCommandList(1, type, *ppAllocator, nullptr, IID_PPV_ARGS(ppCommandList)));
	(*ppCommandList)->SetName(L"CommandList");
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