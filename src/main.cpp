#include "headers.h"


const uint8_t g_framesNum = 3;
bool g_useWarp = false; //Windows Software Rasterization Platform

int g_clientWidth = 1280;
int g_clientHeight = 720;

bool g_isInitialized = false;

//Window Handle
HWND g_hwnd;
RECT g_windowRect;

ComPtr<ID3D12Device2> g_device;
ComPtr<ID3D12CommandQueue> g_commandQueue;
ComPtr<ID3D12GraphicsCommandList> g_commandList;
ComPtr<IDXGISwapChain4> g_swapChain;
ComPtr<ID3D12Resource> g_backBuffers[g_framesNum];
ComPtr<ID3D12CommandAllocator> g_commandAllocators[g_framesNum];
ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
UINT g_RTVDescriptorSize;
UINT g_CurrentBackBufferIndex;

//Synchronization Objects
ComPtr<ID3D12Fence> g_fence;
uint64_t g_fenceValue = 0;
uint64_t g_frameFenceValues[g_framesNum] = {};
HANDLE g_fenceEvent;

//Swap Chain Present Method
bool g_VSync = true; //wait for the next vertical refresh
bool g_tearingSupported = false;
bool g_fullScreen = false;

//Windows Callback Function
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//Process the Command Argument
void ParseCommandLineArguments() {
	int argc;
	wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
	
	for (int i = 0; i < argc; ++i) {
		if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0) {
			g_clientWidth = ::wcstol(argv[++i], nullptr, 10);
		}
		if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0) {
			g_clientHeight = ::wcstol(argv[++i], nullptr, 10);
		}
		if (::wcscmp(argv[i], L"-warp") == 0) {
			g_useWarp = true;
		}
	}

	::LocalFree(argv);
}

//Enable Debug Layer
void EnableDX12DebugLayer() {
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
}

//Check Tearing Support
bool CheckTearingSupport() {
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

//Create Window
HWND CreateWindow(const wchar_t* windowClassName, HINSTANCE hInstance,
	const wchar_t* windowTitle, uint32_t width, uint32_t height) {
	
	//Create window in the center of the displace devcie
	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);
	RECT windowRect = { 0, 0, static_cast<long>(width), static_cast<long>(height) };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	int windowX = std::max<int>(0, (screenWidth - windowWidth));
	int windowY = std::max<int>(0, (screenHeight - windowHeight));

	HWND hWnd = ::CreateWindowExW(
		NULL,
		windowClassName,
		windowTitle,
		WS_OVERLAPPEDWINDOW,
		windowX,
		windowY,
		windowWidth,
		windowHeight,
		NULL,
		NULL,
		hInstance,
		nullptr
	);

	return hWnd;
}


//Query DirectX12 Adapter 
ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp)
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;

#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	// Create DXGI Factory
	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapater1;
	ComPtr<IDXGIAdapter4> dxgiAdapater4;

	if (useWarp)
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


//Create Device
ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter) {
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

//Create Command Queue
ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type) {
	ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));
	return d3d12CommandQueue;
}

//Create Swap Chain
ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd,
	ComPtr<ID3D12CommandQueue> commandQueue,
	uint32_t width, uint32_t height, uint32_t buffercount)
{
	ComPtr<IDXGISwapChain4> swapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //RGBA 32bit
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 }; 
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = buffercount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(commandQueue.Get(), hWnd,
		&swapChainDesc, nullptr, nullptr, &swapChain1));

	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
	ThrowIfFailed(swapChain1.As(&swapChain4));

	return swapChain4;
}

//Create Descriptor Heap
ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors) {

	ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = type;

	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));
	return descriptorHeap;
}

//Create Render Target View
void UpdateRTV(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap)
{
	uint32_t rtvDescriptorIncreSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < g_framesNum; i++) {
		ComPtr<ID3D12Resource> backBuffer;
		//Get buffer from swap chain
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		//Create RTV for each buffer
		device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
		g_backBuffers[i] = backBuffer;
		rtvHandle.Offset(rtvDescriptorIncreSize);
	}
}

//Create Command Allocator
ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type) {
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));
	return commandAllocator;
}

//Create Command List
ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator,
	D3D12_COMMAND_LIST_TYPE type) {
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
	ThrowIfFailed(commandList->Close());
	return commandList;
}

//Create Fence
ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device) {
	ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	return fence;
}

//Create Fence Event Handle: CPU block any further processing until GPU reaches the fence
HANDLE CreateEventHandle() {
	HANDLE fenceEvent;
	fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(fenceEvent && "Failed to create fence event.");
	return fenceEvent;
}

//Wait for GPU to complete the command list
uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence,
	uint64_t& fenceValue) {
	uint64_t fenceValueForSignal = ++fenceValue;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValueForSignal));
	return fenceValueForSignal;
}

//Wait for GPU to complete the command list
void WaiteForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent,
	std::chrono::milliseconds duration = std::chrono::milliseconds::max()) {

	if (fence->GetCompletedValue() < fenceValue)
	{
		//Set the completion event
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
		//Stall the CPU
		::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
	}
}

//Flush the command queue
void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence,
	uint64_t& fenceValue, HANDLE fenceEvent)
{
	uint64_t fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
	WaiteForFenceValue(fence, fenceValue, fenceEvent);
}


//CPU Logic
void Update()
{
	static uint64_t frameCounter = 0;
	static double elapsedSeconds = 0.0f;
	static std::chrono::high_resolution_clock clock;
	static auto t0 = clock.now();

	frameCounter++;
	auto t1 = clock.now();
	auto deltaTime = t1 - t0;
	t0 = t1;

	elapsedSeconds += deltaTime.count() * 1e-9;
	if (elapsedSeconds > 1.0f)
	{
		char buffer[500];
		auto fps = frameCounter / elapsedSeconds;
		sprintf_s(buffer, 500, "FPS: %f\n", fps);
		OutputDebugString(buffer);

		frameCounter = 0;
		elapsedSeconds = 0.0;
	}
}

void Render()
{
	//Get current back buffer
	auto commandAllocator = g_commandAllocators[g_CurrentBackBufferIndex];
	auto backBuffer = g_backBuffers[g_CurrentBackBufferIndex];

	commandAllocator->Reset();
	g_commandList->Reset(commandAllocator.Get(), nullptr);

	// Clear the render target
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		
		g_commandList->ResourceBarrier(1, &barrier);

		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
			g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			g_CurrentBackBufferIndex,
			g_RTVDescriptorSize
		);

		g_commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	}

	// Present
	{
		//create the barrier and transfer the resource to present
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);
		g_commandList->ResourceBarrier(1, &barrier);

		//execute command lists
		ThrowIfFailed(g_commandList->Close());
		ID3D12CommandList* const commandLists[] = {
			g_commandList.Get()
		};
		g_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		//swap chain present 
		UINT syncInterval = g_VSync ? 1 : 0;
		UINT presentFlags = g_tearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
		ThrowIfFailed(g_swapChain->Present(syncInterval, presentFlags));

		//signal fence 
		g_frameFenceValues[g_CurrentBackBufferIndex] = Signal(g_commandQueue, g_fence, g_fenceValue);
		//get the back buffer index
		g_CurrentBackBufferIndex = g_swapChain->GetCurrentBackBufferIndex();
		//waite fence value
		WaiteForFenceValue(g_fence, g_frameFenceValues[g_CurrentBackBufferIndex], g_fenceEvent);
	}
}

//Update resources
void Resize(uint32_t width, uint32_t height) {
	if (g_clientWidth != width || g_clientHeight != height) {
		g_clientWidth = std::max(1u, width);
		g_clientHeight = std::max(1u, height);
		Flush(g_commandQueue, g_fence, g_fenceValue, g_fenceEvent);

		for (int i = 0; i < g_framesNum; ++i) {
			g_backBuffers[i].Reset();
			g_frameFenceValues[i] = g_frameFenceValues[g_CurrentBackBufferIndex];
		}

		//Update Swap chain
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(g_swapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(g_swapChain->ResizeBuffers(g_framesNum, g_clientWidth, g_clientHeight,
			swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		g_CurrentBackBufferIndex = g_swapChain->GetCurrentBackBufferIndex();

		//Update the resources 
		UpdateRTV(g_device, g_swapChain, g_RTVDescriptorHeap);
	}
}

void SetFullScreen(bool fullScreen)
{
	if (g_fullScreen != fullScreen) {
		g_fullScreen = fullScreen;

		if (g_fullScreen) {
			::GetWindowRect(g_hwnd, &g_windowRect);
			UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
			::SetWindowLongW(g_hwnd, GWL_STYLE, windowStyle);

			//Query the nearest window
			HMONITOR hMonitor = ::MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX monitorInfo = {};
			monitorInfo.cbSize = sizeof(MONITORINFOEX);
			::GetMonitorInfo(hMonitor, &monitorInfo);

			//Set the FULL WINDOW
			::SetWindowPos(g_hwnd, HWND_TOP,
				monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.top,
				monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
				monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);
			::ShowWindow(g_hwnd, SW_MAXIMIZE);
		}
		else
		{
			::SetWindowLong(g_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
			::SetWindowPos(g_hwnd, HWND_NOTOPMOST,
				g_windowRect.left,
				g_windowRect.right,
				g_windowRect.right - g_windowRect.left,
				g_windowRect.bottom - g_windowRect.top,
				SWP_FRAMECHANGED | SWP_NOACTIVATE);
			::ShowWindow(g_hwnd, SW_NORMAL);
		}
	}
}

//Windows Message Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (g_isInitialized) {
		switch (message) {
			//rendering
		case WM_PAINT:
			Update();
			Render();
			break;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
			
			switch (wParam) {
				case 'V': {
					g_VSync = !g_VSync;
					break;
				}
				case VK_ESCAPE: {
					::PostQuitMessage(0);
					break;
				}
				case VK_RETURN:
					if (alt) {
				case VK_F11:
					SetFullScreen(!g_fullScreen);
					}
					break;
			}
		}
		break;
		case WM_SYSCHAR:
			break;
		case WM_SIZE:
		{
			//get system window rect
			RECT clientRect = {};
			::GetClientRect(g_hwnd, &clientRect);
			int width = clientRect.right - clientRect.left;
			int height = clientRect.bottom - clientRect.top;
			//resize the directx 12 resources
			Resize(width, height);
		}
		break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			break;
		default:
			return ::DefWindowProcW(hwnd, message, wParam, lParam);
		}
	}
	else {
		return ::DefWindowProcW(hwnd, message, wParam, lParam);
	}
	return 0;
}

//The Main Entry Point
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PWSTR lpCmdLine, int nCmdShow) {

	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);


	const wchar_t* windowClassName = L"DX12WindowClass";
	ParseCommandLineArguments();


	EnableDX12DebugLayer();


	g_tearingSupported = CheckTearingSupport();

	RegisterWindowClass(hInstance, windowClassName);
	g_hwnd = CreateWindow(windowClassName, hInstance, L"Glimmer", g_clientWidth, g_clientHeight);
	::GetWindowRect(g_hwnd, &g_windowRect);

	//Create DirectX12 Objects
	ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(g_useWarp);
	g_device = CreateDevice(dxgiAdapter4);
	g_commandQueue = CreateCommandQueue(g_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	g_swapChain = CreateSwapChain(g_hwnd, g_commandQueue, g_clientWidth, g_clientHeight, g_framesNum);
	g_CurrentBackBufferIndex = g_swapChain->GetCurrentBackBufferIndex();
	g_RTVDescriptorHeap = CreateDescriptorHeap(g_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, g_framesNum);
	g_RTVDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	UpdateRTV(g_device, g_swapChain, g_RTVDescriptorHeap);

	for (int i = 0; i < g_framesNum; ++i) {
		g_commandAllocators[i] = CreateCommandAllocator(g_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	}
	g_commandList = CreateCommandList(g_device, g_commandAllocators[g_CurrentBackBufferIndex],
		D3D12_COMMAND_LIST_TYPE_DIRECT);

	g_fence = CreateFence(g_device);
	g_fenceEvent = CreateEventHandle();


	g_isInitialized = true;

	::ShowWindow(g_hwnd, SW_SHOW);
	MSG msg = {};
	while (msg.message != WM_QUIT) {
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		else //Game Stuff
		{
			Update();
			Render();
		}
	}

	Flush(g_commandQueue, g_fence, g_fenceValue, g_fenceEvent);

	::CloseHandle(g_fenceEvent);
	return 0;
}













