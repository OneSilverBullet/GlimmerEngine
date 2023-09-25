#pragma once


#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_5.h>
#include "events.h"

using namespace Microsoft::WRL;

class Window
{
public:

protected:
	Window(HWND windowInstance, std::wstring windowName, int clientWidth, int clientHeight, bool vSync);


};
