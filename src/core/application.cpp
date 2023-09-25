#include "application.h"
#include "headers.h"
#include "window.h"
#include "game.h"
#include "commandqueue.h"

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
static WindowMap g_windowMap;
static WindowNameMap g_windowNameMap;


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
		assert(g_windowNameMap.empty() && g_windowMap.empty());
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
		m_directCommandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_computeCommandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
		m_copyCommandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_COPY);
		m_tearingSupported = CheckTearingSupport();
	}
}

Application::~Application(){
	Flush();
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

			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
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
	return allowTearing == TRUE;
}

std::shared_ptr<Window> Application::CreateRenderWindow(const std::wstring& windowName,
	int clientWidth, int clientHeight, bool vSync = true) {

	if (g_windowNameMap.find(windowName) != g_windowNameMap.end()) {
		return g_windowNameMap[windowName];
	}

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

	g_windowMap[hWnd] = pWindow;
	g_windowNameMap[windowName] = pWindow;
}


void Application::DestroyWindow(const std::wstring& windowName) {

}

void Application::DestroyWindow(std::shared_ptr<Window> window) {
	
}

std::shared_ptr<Window> Application::GetWindowByName(const std::wstring& windowName) {
	return g_windowNameMap[windowName];
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
	}

	Flush();

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

std::shared_ptr<CommandQueue> Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) {
	if (type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
		return m_computeCommandQueue;
	else if (type == D3D12_COMMAND_LIST_TYPE_COPY)
		return m_copyCommandQueue;
	else if (type == D3D12_COMMAND_LIST_TYPE_DIRECT)
		return m_directCommandQueue;
	return nullptr;
}

void Application::Flush() {
	m_directCommandQueue->Flush();
	m_computeCommandQueue->Flush();
	m_copyCommandQueue->Flush();
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




