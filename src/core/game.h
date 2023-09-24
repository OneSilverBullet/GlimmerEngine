#include <string>
#include <Windows.h>
#include <memory>
#include "events.h"

class Window;

class Game : public std::enable_shared_from_this<Game> {

public:
	Game();
	virtual ~Game();
	int GetClientWidth() const;
	int GetClientHeight() const;
	bool GetVSync() const;
	virtual bool Initialize();
	virtual bool LoadContent(void) = 0;
	virtual void UnloadContent(void) = 0;
	virtual void Destroy(void);

protected:
	friend class Window;

	virtual void OnUpdate(UpdateEventArgs& args);
	virtual void OnRender(RenderEventArgs& args);
	virtual void OnKeyPressed(KeyEventArgs& args);
	virtual void OnKeyReleased(KeyEventArgs& args);
	virtual void OnMouseMoved(MouseMotionEventArgs& args);
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& args);
	virtual void OnMouseWheel(MouseWheelEventArgs& args);
	virtual void OnResize(ResizeEventArgs& args);
	virtual void OnWindowDestroy();

	std::shared_ptr<Window> m_window;

private:
	std::string name;
	int m_clientWidth;
	int m_clientHeight;
	bool m_vsync;
};



