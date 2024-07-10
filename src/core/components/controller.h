#pragma once
#include <vector>
#include <string>
#include "events.h"
#include "camera.h"

class Controller
{
public:
	Controller();

	void ProcessMouseMoveEvent(MouseMotionEventArgs& e);
	void BindCamera(Camera* cam) { m_bindingCamera = cam; }
	void UnbindCamera() { m_bindingCamera = nullptr; }

private:
	Camera* m_bindingCamera;
	int m_currentX;
	int m_currentY;
	int m_previousX;
	int m_previousY;
};

