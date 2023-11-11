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

	//Register window class
	RegisterWindowClass(m_hInst, L"Glimmer");

	GRAPHICS_CORE::GraphicsCoreInitialize();


	//Initialize Timer
	m_timer = new EngineTimer();
}

Application::~Application(){
	GRAPHICS_CORE::GraphicsCoreRelease();
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


