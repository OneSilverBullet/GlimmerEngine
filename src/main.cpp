#include "headers.h"
#include "window.h"
#include "application.h"
#include "clientgame.h"
#include <dxgidebug.h>
#include <shlwapi.h>
#include <iostream>


//Check the object leak
void ReportLiveObjects() {
	IDXGIDebug1* dxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
	dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	dxgiDebug->Release();
}

//Set work directory
int SetWorkDirectory() {
	WCHAR path[MAX_PATH];
	HMODULE hm = GetModuleHandleW(NULL);
	GetModuleFileNameW(hm, path, MAX_PATH);
	PathRemoveFileSpecW(path);
	SetCurrentDirectoryW(path);
	return 0;
}


//The Main Entry Point
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PWSTR lpCmdLine, int nCmdShow) {

	//create console in windows system
	AllocConsole();

	//redirect the std iostream into current console
	FILE* stream;
	freopen_s(&stream, "CONOUT$", "w", stdout);
	freopen_s(&stream, "CONOUT$", "w", stderr);
	freopen_s(&stream, "CONIN$", "r", stdin);

	std::cout << "Glimmer Engine Console" << std::endl;


	SetWorkDirectory();
	Application::Initialize(hInstance);
	std::shared_ptr<ClientGame> game = std::make_shared<ClientGame>(L"Glimmer", 1280, 720);
	Application::GetInstance().Run(game);
	Application::Release();
	atexit(&ReportLiveObjects);
	return 0;
}













