#pragma once


#include <d3d12.h>
#include <dxgi1_6.h>

#include <wrl.h>
#include <memory>
#include <string>

class Window;
class CommandQueue;
class Game;
class EngineTimer;

using namespace Microsoft::WRL;

class Application
{
public:
	static void Initialize(HINSTANCE hInst);
	static void Release();
	static Application& GetInstance();
	bool IsTearingSupported() const;
	std::shared_ptr<Window> CreateRenderWindow(const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync = true);
	void DestroyWindow(const std::wstring& windowName);
	void DestroyWindow(std::shared_ptr<Window> window);
	std::shared_ptr<Window> GetWindowByName();
	
	int Run(std::shared_ptr<Game> gameInstance);
	void Quit(int exitCode = 0);

	ComPtr<ID3D12Device2> GetDevice() const;

	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type);
	UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

protected:
	Application(HINSTANCE hInst);
	virtual ~Application();

	ComPtr<IDXGIAdapter4> GetAdapter(bool bUseWarp);
	ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter);
	bool CheckTearingSupport();

private:
	Application(const Application& copy) = delete;
	Application& operator=(const Application& other) = delete;

	HINSTANCE m_hInst;

	EngineTimer* m_timer = nullptr;

	ComPtr<IDXGIAdapter4> m_dxgiAdapter;
	ComPtr<ID3D12Device2> m_device;

	bool m_tearingSupported;
};




