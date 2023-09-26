#include "game.h"
#include "headers.h"
#include "application.h"
#include "window.h"

Game::Game(const std::wstring& name, int width, int height, bool vsync):
	m_name(name), m_clientWidth(width), m_clientHeight(height), m_vsync(vsync)
{

}

Game::~Game() {
	assert(!m_window && "Use Game::Destroy() before destruction.");
}

int Game::GetClientWidth() const { return m_clientWidth; }

int Game::GetClientHeight() const { return m_clientHeight; }

bool Game::GetVSync() const { return m_vsync; }

bool Game::Initialize() {

	//Check the support directX mathlib
	if (!DirectX::XMVerifyCPUSupport()) {
		return false;
	}

	//Create the window from the application
	m_window = Application::GetInstance().CreateRenderWindow(m_name, m_clientWidth, m_clientHeight, m_vsync);
	m_window->RegisterCallbacks(shared_from_this());
	m_window->Show();

	return true;
}

void Game::Destroy(void) {
	Application::GetInstance().DestroyWindow(m_window);
	m_window.reset();
}


void Game::OnUpdate(UpdateEventArgs& args) {

}

void Game::OnRender(RenderEventArgs& args) {

}

void Game::OnKeyPressed(KeyEventArgs& args) {

}

void Game::OnKeyReleased(KeyEventArgs& args) {

}

void Game::OnMouseMoved(MouseMotionEventArgs& args) {

}

void Game::OnMouseButtonPressed(MouseButtonEventArgs& args) {

}

void Game::OnMouseButtonReleased(MouseButtonEventArgs& args) {

}

void Game::OnMouseWheel(MouseWheelEventArgs& args) {

}

void Game::OnResize(ResizeEventArgs& args) {
	m_clientWidth = args.Width;
	m_clientHeight = args.Height;
}

void Game::OnWindowDestroy() {
	UnloadContent();
}