#include "game.h"
#include "headers.h"

Game::Game() {

}

Game::~Game() {

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

}

void Game::Destroy(void) {
	//Create the window from the application
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

void Game::OnMouseWheel(MouseWheelEventArgs& args) {

}

void Game::OnResize(ResizeEventArgs& args) {
	m_clientWidth = args.Width;
	m_clientHeight = args.Height;
}

void Game::OnWindowDestroy() {
	UnloadContent();
}