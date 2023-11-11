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


protected:
	Application(HINSTANCE hInst);
	virtual ~Application();

private:
	Application(const Application& copy) = delete;
	Application& operator=(const Application& other) = delete;

	HINSTANCE m_hInst;

	EngineTimer* m_timer = nullptr;

	bool m_tearingSupported;
};




