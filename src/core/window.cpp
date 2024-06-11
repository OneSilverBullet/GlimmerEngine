#include "window.h"
#include "headers.h"
#include "commandqueue.h"
#include "graphicscore.h"
#include "game.h"
#include "application.h"


Window::Window(HWND windowInstance, std::wstring windowName,
	int clientWidth, int clientHeight, bool vSync) :
	m_hWnd(windowInstance), m_windowName(windowName),
	m_clientWidth(clientWidth), m_clientHeight(clientHeight),
	m_vSync(vSync), m_fullscreen(false)
{
	Application& app = Application::GetInstance();

	m_isTearingSupported = app.IsTearingSupported();
	m_dxgiSwapChain = CreateSwapChain();

	UpdateRenderTargetViews();
}

Window::~Window() {
	assert(!m_hWnd && "Use Application::DestroyWindow before destruction.");
}

HWND Window::GetWindowHandle() const {
	return m_hWnd;
}

void Window::Destroy() {
	auto pGame = m_pGame.lock();
	assert(pGame && "the game is destoryed before destruction");
	pGame->OnWindowDestroy();
	
	if (m_hWnd) {
		DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
	}
}

std::wstring Window::GetWindowName() const { return m_windowName; }

int Window::GetClientWidth() const {
	return m_clientWidth;
}

int Window::GetClientHeight() const {
	return m_clientHeight;
}

//vertical refresh synchronization
bool Window::IsVSync() const {
	return m_vSync;
}

void Window::SetVSync(bool vSync) {
	m_vSync = vSync;
}

void Window::ToggleVSync() {
	m_vSync = !m_vSync;
}

//full-screen
bool Window::IsFullScreen() const {
	return m_fullscreen;
}

void Window::SetFullscreen(bool fullscreen) {
	if (m_fullscreen != fullscreen) {
		m_fullscreen = fullscreen;

		if (m_fullscreen) {
			::GetWindowRect(m_hWnd, &m_windowRect);
			UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
			::SetWindowLongW(m_hWnd, GWL_STYLE, windowStyle);

			//Query the nearest window
			HMONITOR hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX monitorInfo = {};
			monitorInfo.cbSize = sizeof(MONITORINFOEX);
			::GetMonitorInfo(hMonitor, &monitorInfo);

			//Set the FULL WINDOW
			::SetWindowPos(m_hWnd, HWND_TOP,
				monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.top,
				monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);
			::ShowWindow(m_hWnd, SW_MAXIMIZE);
		}
		else
		{
			::SetWindowLong(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
			::SetWindowPos(m_hWnd, HWND_NOTOPMOST,
				m_windowRect.left,
				m_windowRect.right,
				m_windowRect.right - m_windowRect.left,
				m_windowRect.bottom - m_windowRect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);
			::ShowWindow(m_hWnd, SW_NORMAL);
		}
	}
}

void Window::ToggleFullscreen() {
	m_fullscreen = !m_fullscreen;
}

//show and hide
void Window::Show() {
	::ShowWindow(m_hWnd, SW_SHOW);
	::UpdateWindow(m_hWnd);
}

void Window::Hide() {
	::ShowWindow(m_hWnd, SW_HIDE);
}

void Window::RegisterCallbacks(std::shared_ptr<Game> pGame) {
	m_pGame = pGame;
}

void Window::OnUpdate(UpdateEventArgs& e) {
	auto pGame = m_pGame.lock();
	if (pGame) {
		pGame->OnUpdate(e);
	}
}

void Window::OnRender(RenderEventArgs& e) {
	auto pGame = m_pGame.lock();
	if (pGame) {
		pGame->OnRender(e);
	}
}

void Window::OnKeyPressed(KeyEventArgs& e) {
	auto pGame = m_pGame.lock();
	if (pGame) {
		pGame->OnKeyPressed(e);
	}
}

void Window::OnKeyReleased(KeyEventArgs& e) {
	auto pGame = m_pGame.lock();
	if (pGame) {
		pGame->OnKeyReleased(e);
	}
}

void Window::OnMouseMoved(MouseMotionEventArgs& e) {
	auto pGame = m_pGame.lock();
	if (pGame) {
		pGame->OnMouseMoved(e);
	}
}

void Window::OnMouseButtonPressed(MouseButtonEventArgs& e) {
	auto pGame = m_pGame.lock();
	if (pGame) {
		pGame->OnMouseButtonPressed(e);
	}
}

void Window::OnMouseButtonReleased(MouseButtonEventArgs& e) {
	auto pGame = m_pGame.lock();
	if (pGame) {
		pGame->OnMouseButtonReleased(e);
	}
}

void Window::OnMouseWheel(MouseWheelEventArgs& e) {
	auto pGame = m_pGame.lock();
	if (pGame) {
		pGame->OnMouseWheel(e);
	}
}

void Window::OnResize(ResizeEventArgs& e) {
	if (m_clientWidth != e.Width || m_clientHeight != e.Height) {
		m_clientWidth = 1u < e.Width ? e.Width : 1u;
		m_clientHeight = 1u < e.Height ? e.Height : 1u;

		GRAPHICS_CORE::g_commandManager.Flush();

		for (int i = 0; i < BufferCount; ++i) {
			m_backbuffer[i].Destroy();
		}

		//Update Swap chain
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(m_dxgiSwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(m_dxgiSwapChain->ResizeBuffers(BufferCount, m_clientWidth, m_clientHeight,
			swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		m_currentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

		//Update the resources 
		UpdateRenderTargetViews();
	}
}

ComPtr<IDXGISwapChain4> Window::CreateSwapChain() {


	ComPtr<IDXGISwapChain4> swapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = m_clientWidth;
	swapChainDesc.Height = m_clientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //RGBA 32bit
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = BufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = m_isTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;


	ID3D12CommandQueue* commandQueue = GRAPHICS_CORE::g_commandManager.GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT).GetCommandQueue();
 
	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(commandQueue, m_hWnd,
		&swapChainDesc, nullptr, nullptr, &swapChain1));

	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER));
	ThrowIfFailed(swapChain1.As(&swapChain4));

	m_currentBackBufferIndex = swapChain4->GetCurrentBackBufferIndex();

	return swapChain4;
}

void Window::UpdateRenderTargetViews() {
	auto device = GRAPHICS_CORE::g_device;
	uint32_t rtvDescriptorIncreSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	for (int i = 0; i < BufferCount; i++) {
		ComPtr<ID3D12Resource> backBuffer;
		//Get buffer from swap chain
		ThrowIfFailed(m_dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		
		m_backbuffer[i].CreateFromSwapChain(L"backbuffer", backBuffer.Detach());
	}
}

UINT Window::GetCurrentBackBufferIndex() const {
	return m_currentBackBufferIndex;
}

UINT Window::Present() {
	//swap chain present 
	UINT syncInterval = m_vSync ? 1 : 0;
	UINT presentFlags = m_isTearingSupported && !m_vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;

	//present the back buffer to the screen
	ThrowIfFailed(m_dxgiSwapChain->Present(syncInterval, presentFlags));

	//get the back buffer index from swap chain
	m_currentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex(); 
	
	return m_currentBackBufferIndex;
}

D3D12_CPU_DESCRIPTOR_HANDLE Window::GetCurrentRenderTargetView() const {
	return m_backbuffer[m_currentBackBufferIndex].GetRTV();
}

ID3D12Resource* Window::GetCurrentBackBufferRaw() {
	return m_backbuffer[m_currentBackBufferIndex].GetResource();
}


ColorBuffer& Window::GetCurrentBackBuffer() {
	return m_backbuffer[m_currentBackBufferIndex];
}
