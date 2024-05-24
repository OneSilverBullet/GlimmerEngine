#pragma once
#include <d3d12.h>
#include "commandmanager.h"
#include <vector>
#include <memory>


class Context;

class ContextManager
{
public:
	ContextManager(){}


private:
	std::vector<std::unique_ptr<Context>> m_contextPool[4];
	std::queue<Context*> m_availableContextPool[4];



};

//context class limitation
struct NonCopyable
{
	NonCopyable() = delete;
	NonCopyable(const NonCopyable& v) = delete;
	NonCopyable& operator=(const NonCopyable& v) = delete;
};

//basic context class
class Context : public NonCopyable
{
public:
	  

protected:
	ID3D12CommandAllocator* m_commandAllocator;
	ID3D12GraphicsCommandList* m_graphicsCommandList;

	ID3D12RootSignature* m_renderSignature;
	ID3D12RootSignature* m_computeSignature;
	ID3D12PipelineState* m_pipelineState;







};



