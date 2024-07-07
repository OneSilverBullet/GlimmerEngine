
#include "game.h"
#include "window.h"
#include <DirectXMath.h>
#include "resources/depthbuffer.h"
#include "resources/byteaddressbuffer.h"
#include "texturemanager.h"
#include "staticdecriptorheap.h"
#include "components/skybox.h"
#include "components/hdrtocubemap.h"

class RootSignature;
class GraphicsPSO;


class ClientGame : public Game
{
public:
	using super = Game;

	ClientGame(const std::wstring& name, int width, int height, bool vSync = false);
	virtual ~ClientGame();
	virtual bool LoadContent() override;
	virtual void UnloadContent() override;

protected:
	virtual void OnUpdate(UpdateEventArgs& e) override;
	virtual void OnRender(RenderEventArgs& e) override;
	virtual void OnKeyPressed(KeyEventArgs& e) override;
	virtual void OnKeyReleased(KeyEventArgs& e) override;
	virtual void OnMouseMoved(MouseMotionEventArgs& e) override;
	virtual void OnMouseWheel(MouseWheelEventArgs& e) override;
	virtual void OnResize(ResizeEventArgs& e) override;
	virtual void OnWindowDestroy() override;

private:


	void ResizeDepthBuffer(int width, int height);


	SkyBox m_skybox;
	HDRLoader m_hdrLoader;

	DepthBuffer m_depthBuffer;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	float m_fov;
	float m_aspectRatio;

	DirectX::XMMATRIX m_worldMatrix;
	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_projMatrix;

	bool m_contentLoaded;
};


