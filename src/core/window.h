#pragma once


#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <string>
#include <memory>
#include "events.h"

using namespace Microsoft::WRL;

class Game;

class Window
{
public:
	static const UINT BufferCount = 3;

	HWND GetWindowHandle() const;
	void Destroy();
	std::wstring GetWindowName() const;
	int GetClientWidth() const;
	int GetClientHeight() const;
	//vertical refresh synchronization
	bool IsVSync() const;
	void SetVSync(bool vSync);
	void ToggleVSync();
	//full-screen
	bool IsFullScreen() const;
	void SetFullscreen(bool fullscreen);
	void ToggleFullscreen();
	//show and hide
	void Show();
	void Hide();

	UINT GetCurrentBackBufferIndex() const;
	UINT Present();
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;
	ComPtr<ID3D12Resource> GetCurrentBackBuffer() const;

protected:
	friend LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	friend class Application;
	friend class Game;

	Window() = delete;
	Window(HWND windowInstance, std::wstring windowName, int clientWidth, int clientHeight, bool vSync);
	virtual ~Window();

	void RegisterCallbacks(std::shared_ptr<Game> pGame);

	virtual void OnUpdate(UpdateEventArgs& e);
	virtual void OnRender(RenderEventArgs& e);
	virtual void OnKeyPressed(KeyEventArgs& e);
	virtual void OnKeyReleased(KeyEventArgs& e);
	virtual void OnMouseMoved(MouseMotionEventArgs& e);
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& e);
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& e);
	virtual void OnMouseWheel(MouseWheelEventArgs& e);
	virtual void OnResize(ResizeEventArgs& e);

	ComPtr<IDXGISwapChain4> CreateSwapChain();
	void UpdateRenderTargetViews();

private:
	Window(const Window& copy) = delete;
	Window& operator=(const Window& other) = delete;


	std::wstring m_windowName;
	int m_clientWidth;
	int m_clientHeight;
	bool m_vSync;
	bool m_fullscreen;

	std::weak_ptr<Game> m_pGame;

	HWND m_hWnd;
	ComPtr<IDXGISwapChain4> m_dxgiSwapChain;
	ComPtr<ID3D12DescriptorHeap> m_d3d12RTVDescriptorHeap;
	ComPtr<ID3D12Resource> m_backbuffers[BufferCount];
	UINT m_RTVDescriptorSize;
	UINT m_currentBackBufferIndex;

	RECT m_windowRect;
	bool m_isTearingSupported;


};
