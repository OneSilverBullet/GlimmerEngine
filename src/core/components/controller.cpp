#include "controller.h"


Controller::Controller(): m_bindingCamera(nullptr), 
m_previousX(-1), m_previousY(-1), m_currentX(-1), m_currentY(-1)
{
}

void Controller::ProcessMouseMoveEvent(MouseMotionEventArgs& e)
{
	//to do: camera should be a character, in an earlier version, we just control the camera
	if (m_bindingCamera != nullptr)
	{
		m_currentX = e.X;
		m_currentY = e.Y;

		int offsetX = 0;
		int offsetY = 0;

		if (m_previousX != -1) offsetX = m_previousX - m_currentX;
		if (m_previousY != -1) offsetY = m_previousY - m_currentY;

		m_bindingCamera->RotateCamera(offsetX, offsetY);

		m_previousX = m_currentX;
		m_previousY = m_currentY;
	}
}




