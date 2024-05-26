#pragma once
#include "context.h"

/*
* ContextManager
*/
Context* ContextManager::AllocateContext(D3D12_COMMAND_LIST_TYPE type)
{
	std::lock_guard<std::mutex> lockGuard(sm_contextAllocatorMutex);

	auto& availableContexts = m_availableContextPool[type];

	Context* ret = nullptr;
	if (availableContexts.empty()) {

	}
	else {
		ret = availableContexts.front();
		availableContexts.pop();
		
	}


	return nullptr;
}

void ContextManager::FreeContext(Context*)
{
}

void ContextManager::DestroyAllContexts()
{
}
