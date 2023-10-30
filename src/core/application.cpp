#include "application.h"
#include "headers.h"
#include "window.h"
#include "game.h"
#include "commandqueue.h"
#include "events.h"
#include "timer.h"
#include "graphicscore.h"
#include <unordered_map>


struct WindowInstance : public Window
{
	WindowInstance(HWND windowInstance, std::wstring windowName, int clientWidth, int clientHeight, bool vsync) :
		Window(windowInstance, windowName, clientWidth, clientHeight, vsync)
	{
	}
};

using WindowPtr = std::shared_ptr<WindowInstance>;
using WindowMap = std::unordered_map<HWND, WindowPtr>;
using WindowNameMap = std::unordered_map<std::wstring, WindowPtr>;

static Application* g_application = nullptr;
static WindowPtr g_windowPtr = nullptr;

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


void EnableDX12DebugLayer() {
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
}

//Regist Window
void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName) {

	WNDCLASSEXW windowClass = {};
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = &WndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInst;
	windowClass.hIcon = ::LoadIcon(hInst, NULL);
	windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = windowClassName;
	windowClass.hIconSm = ::LoadIcon(hInst, NULL);

	static ATOM atom = ::RegisterClassExW(&windowClass);
	assert(atom > 0);
}

void Application::Initialize(HINSTANCE hInst) {
	if (!g_application) {
		g_application = new Application(hInst);
	}
}
void Application::Release() {
	
	if (g_application != nullptr) {
		assert(g_windowPtr == nullptr);
		delete g_application;
		g_application = nullptr;
	}
}

Application& Application::GetInstance() {
	assert(g_application != nullptr);
	return *g_application;
}

Application::Application(HINSTANCE hInst)
	: m_hInst(hInst), m_tearingSupported(false)
{
	//adapte to the high dpi context
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	//Enable debug layer
	EnableDX12DebugLayer();

	//Register window class
	RegisterWindowClass(m_hInst, L"Glimmer");

	//Initialize DX12 Objects
	m_dxgiAdapter = GetAdapter(false);

	if (m_dxgiAdapter) {
		m_device = CreateDevice(m_dxgiAdapter);
	}
	if (m_device) {

		
		GRAPHICS_CORE::g_commandManager.Initialize(m_device.Get());

		//m_directCommandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		//m_computeCommandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
		//m_copyCommandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_COPY);
		m_tearingSupported = CheckTearingSupport();
	}

	//Initialize Timer
	m_timer = new EngineTimer();
}

Application::~Application(){
	m_device->Release();
}

ComPtr<IDXGIAdapter4> Application::GetAdapter(bool bUseWarp) {
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;

#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	// Create DXGI Factory
	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapater1;
	ComPtr<IDXGIAdapter4> dxgiAdapater4;

	if (bUseWarp)
	{
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapater1)));
		ThrowIfFailed(dxgiAdapater1.As(&dxgiAdapater4));
	}
	else
	{
		SIZE_T maxDedicatedVideoMeory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapater1) != DXGI_ERROR_NOT_FOUND; i++) {
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapater1->GetDesc1(&dxgiAdapterDesc1);

			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapater1.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMeory) {
				maxDedicatedVideoMeory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapater1.As(&dxgiAdapater4));
			}
		}
	}
	return dxgiAdapater4;
}

ComPtr<ID3D12Device2> Application::CreateDevice(ComPtr<IDXGIAdapter4> adapter) {
	ComPtr<ID3D12Device2> device;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

	
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(device.As(&pInfoQueue))) {
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		D3D12_MESSAGE_SEVERITY severities[] = {
			D3D12_MESSAGE_SEVERITY_INFO
		};

		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
		};

		D3D12_INFO_QUEUE_FILTER newFilter = {};
		newFilter.DenyList.NumSeverities = _countof(severities);
		newFilter.DenyList.pSeverityList = severities;
		newFilter.DenyList.NumIDs = _countof(denyIds);
		newFilter.DenyList.pIDList = denyIds;
		ThrowIfFailed(pInfoQueue->PushStorageFilter(&newFilter));
	}
#endif

	return device;
}

bool Application::CheckTearingSupport() {
	BOOL allowTearing = FALSE;

	ComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4)))) {
		ComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory4.As(&factory5))) {
			if (FAILED(factory5->CheckFeatureSupport(
				DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&allowTearing, sizeof(allowTearing)))) {
				allowTearing = FALSE;
			}
		}
	}
	m_tearingSupported = allowTearing;
	return allowTearing == TRUE;
}

bool Application::IsTearingSupported() const {
	return m_tearingSupported;
}

std::shared_ptr<Window> Application::CreateRenderWindow(const std::wstring& windowName,
	int clientWidth, int clientHeight, bool vSync) {

	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);
	RECT windowRect = { 0, 0, static_cast<long>(clientWidth), static_cast<long>(clientHeight) };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	HWND hWnd = CreateWindowW(
		L"Glimmer",
		windowName.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowWidth, windowHeight,
		nullptr, nullptr, 
		m_hInst, nullptr);

	assert(hWnd && "Failed to create window");

	std::shared_ptr<WindowInstance> pWindow = std::make_shared<WindowInstance>(hWnd, 
		windowName, clientWidth, clientHeight, vSync);

	g_windowPtr = pWindow;

	return pWindow;
}


void Application::DestroyWindow(const std::wstring& windowName) {
	DestroyWindow(g_windowPtr);
}

void Application::DestroyWindow(std::shared_ptr<Window> window) {
	if (window) {
		window->Destroy();
		g_windowPtr = nullptr;
	}
}

std::shared_ptr<Window> Application::GetWindowByName() {
	return g_windowPtr;
}

int Application::Run(std::shared_ptr<Game> gameInstance) {
	if (!gameInstance->Initialize()) return 1;
	if(!gameInstance->LoadContent()) return 2;

	MSG msg = { 0 };



	while (msg.message != WM_QUIT) {
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		else
		{
			double totalTime = m_timer->TotalTime();
			UpdateEventArgs updateEventArgs(0.0f, totalTime);
			g_windowPtr->OnUpdate(updateEventArgs);
			RenderEventArgs renderEventArgs(0.0f, 0.0f);
			g_windowPtr->OnRender(renderEventArgs);
		}
	}

	GRAPHICS_CORE::g_commandManager.Flush();

	gameInstance->UnloadContent();
	gameInstance->Destroy();

	return static_cast<int>(msg.wParam);

}

void Application::Quit(int exitCode) {
	PostQuitMessage(exitCode);
}

ComPtr<ID3D12Device2> Application::GetDevice() const {
	return m_device;
}

ComPtr<ID3D12DescriptorHeap> Application::CreateDescriptorHeap(UINT numDescriptors, 
	D3D12_DESCRIPTOR_HEAP_TYPE type) {
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0; //default GPU
	desc.NumDescriptors = numDescriptors;
	desc.Type = type;

	ThrowIfFailed(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));
	return descriptorHeap;
}

UINT Application::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const {
	return m_device->GetDescriptorHandleIncrementSize(type);
}

//anaylisis the mouse button type
MouseButtonEventArgs::MouseButton DecodeMouseButton(UINT messageID) {
	MouseButtonEventArgs::MouseButton mouseButton = MouseButtonEventArgs::None;
	switch (messageID) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	{
		mouseButton = MouseButtonEventArgs::Left;
		break;
	}
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
	case WM_RBUTTONUP:
	{
		mouseButton = MouseButtonEventArgs::Right;
		break;
	}
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
	case WM_MBUTTONUP:
	{
		mouseButton = MouseButtonEventArgs::Middel;
		break;
	}
	}
	return mouseButton;
}


//Windows Message Procedure
static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	if (g_windowPtr) {
		switch (message) {
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			MSG charmsg;
			unsigned  int c = 0;
			if (PeekMessage(&charmsg, hwnd, 0, 0, PM_NOREMOVE) && charmsg.message == WM_CHAR) {
				GetMessage(&charmsg, hwnd, 0, 0);
				c = static_cast<unsigned int>(charmsg.wParam);
			}

			bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
			bool control = (::GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			bool shift = (::GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
			KeyCode::Key key = (KeyCode::Key)wParam;

			KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Pressed, alt, control, shift);
			g_windowPtr->OnKeyPressed(keyEventArgs);
		}
		break;
		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
			bool control = (::GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			bool shift = (::GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
			KeyCode::Key key = (KeyCode::Key)wParam;
			unsigned int c = 0;
			unsigned int scanCode = (lParam & 0x00FF0000) >> 16;

			unsigned char keyboardState[256];
			bool keyres = GetKeyboardState(keyboardState);
			wchar_t translatedCharacters[4];
			if (int result = ToUnicodeEx(static_cast<UINT>(wParam), scanCode,
				keyboardState, translatedCharacters, 4, 0, NULL) > 0) {
				c = translatedCharacters[0];
			}
			KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Released, alt, control, shift);
			g_windowPtr->OnKeyReleased(keyEventArgs);
		}
		break;
		case WM_SYSCHAR:
			break;
		case WM_MOUSEMOVE:{
			bool lButton = (wParam & MK_LBUTTON) != 0;
			bool rButton = (wParam & MK_RBUTTON) != 0;
			bool mButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool control = (wParam & MK_CONTROL) != 0;
			int x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
			int y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
			MouseMotionEventArgs mouseMotionEventArgs(lButton, mButton, rButton, control, shift, x, y, x, y);
			g_windowPtr->OnMouseMoved(mouseMotionEventArgs);
		}
		break;
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		{
			bool lButton = (wParam & MK_LBUTTON) != 0;
			bool rButton = (wParam & MK_RBUTTON) != 0;
			bool mButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool control = (wParam & MK_CONTROL) != 0;
			int x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
			int y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
			MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Pressed, lButton, mButton, rButton, control, shift, x, y);
			g_windowPtr->OnMouseButtonPressed(mouseButtonEventArgs);
		}
		break;
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		{
			bool lButton = (wParam & MK_LBUTTON) != 0;
			bool rButton = (wParam & MK_RBUTTON) != 0;
			bool mButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool control = (wParam & MK_CONTROL) != 0;
			int x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
			int y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
			MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Released, lButton, mButton, rButton, control, shift, x, y);
			g_windowPtr->OnMouseButtonPressed(mouseButtonEventArgs);
		}
		break;
		case WM_MOUSEWHEEL:
		{
			float zDelta = static_cast<int>(static_cast<short>HIWORD(wParam)) / (float)WHEEL_DELTA;
			short keyStates = (short)LOWORD(wParam);
			bool lButton = (wParam & MK_LBUTTON) != 0;
			bool rButton = (wParam & MK_RBUTTON) != 0;
			bool mButton = (wParam & MK_MBUTTON) != 0;
			bool shift = (wParam & MK_SHIFT) != 0;
			bool control = (wParam & MK_CONTROL) != 0;
			int x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
			int y = static_cast<int>(static_cast<short>(HIWORD(lParam)));
			POINT clientToScreenPoint{ x, y };
			ScreenToClient(hwnd, &clientToScreenPoint);
			MouseWheelEventArgs mouseWheelEventArgs(zDelta, 
				lButton, mButton, rButton, control, shift, x, y);
			g_windowPtr->OnMouseWheel(mouseWheelEventArgs);
		}
		break;
		case WM_SIZE:
		{
			int x = static_cast<int>(static_cast<short>(LOWORD(lParam)));
			int y = static_cast<int>(static_cast<short>(HIWORD(lParam)));

			ResizeEventArgs resizeEventArgs(x, y);
			g_windowPtr->OnResize(resizeEventArgs);
		}
		break;
		case WM_DESTROY: {
			::PostQuitMessage(0);
			break;
		}
		default:
			return ::DefWindowProcW(hwnd, message, wParam, lParam);
		}
	}
	else {
		return ::DefWindowProcW(hwnd, message, wParam, lParam);
	}
	return 0;
}


