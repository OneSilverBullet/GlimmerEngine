#pragma once
#include <vector>
#include <string>
#include <DirectXMath.h>

class Camera
{
public:
	Camera() : camera_fov(DirectX::XM_PIDIV4), camera_aspect(1.33), camera_nearZ(0.1f), camera_farZ(100.0f) {}
	virtual DirectX::XMMATRIX GetViewMatrix() = 0;
	virtual DirectX::XMMATRIX GetViewMatrixForSkybox() = 0;
	virtual void RotateCamera(float xoffset, float yoffset) = 0;
	virtual ~Camera() {}
	virtual void SetTarget(DirectX::XMFLOAT3) {}
	virtual void AddDistance(float dt) {}
	virtual void DecDistance(float dt) {}
	virtual DirectX::XMFLOAT3 GetPosition() = 0;

	DirectX::XMMATRIX GetProjMatrix()
	{
		DirectX::XMMATRIX projMatrix = DirectX::XMMatrixPerspectiveFovLH(camera_fov, camera_aspect, camera_nearZ, camera_farZ);
		return projMatrix;
	}
public:
	float GetFov() { return camera_fov; }
	float GetAspect() { return camera_aspect; }
	float GetNearZ() { return camera_nearZ; }
	float GetFarZ() { return camera_farZ; }

private:
	float camera_fov;
	float camera_aspect;
	float camera_nearZ;
	float camera_farZ;


};

class ThirdRoleCamera : public Camera
{
public:
	ThirdRoleCamera() :Camera() {}

	void InitializeThirdRoleCamera();
	void RotateCamera(float xoffset, float yoffset);

	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMMATRIX GetViewMatrix();
	DirectX::XMMATRIX GetViewMatrixForSkybox();

	void SetTarget(DirectX::XMFLOAT3 tar) { thirdcamera_target = tar; }
public:
	DirectX::XMFLOAT3 GetForward() {
		DirectX::XMFLOAT3 position = GetPosition();
		DirectX::XMVECTOR position_xm = DirectX::XMLoadFloat3(&position);
		DirectX::XMVECTOR target_xm = DirectX::XMLoadFloat3(&thirdcamera_target);
		DirectX::XMVECTOR difference_xm = DirectX::XMVectorSubtract(target_xm, position_xm);
		DirectX::XMVECTOR forward_xm = DirectX::XMVector3Normalize(difference_xm);
		DirectX::XMFLOAT3 forward;
		DirectX::XMStoreFloat3(&forward, forward_xm);
		return forward;
	}
	float GetCameraYaw() { return thirdcamera_yaw; }
	float GetCameraPitch() { return thirdcamera_pitch; }
	void AddDistance(float dt) {
		thirdcamera_distance += dt;
		if (thirdcamera_distance > 10.0f) thirdcamera_distance = 10.0f;
	}
	void DecDistance(float dt) {
		thirdcamera_distance -= dt;
		if (thirdcamera_distance < 2.0f) thirdcamera_distance = 2.0f;
	}
private:
	DirectX::XMFLOAT3 thirdcamera_target;
	DirectX::XMFLOAT3 thirdcamera_up;
	float thirdcamera_sensity;
	float thirdcamera_distance;
	float thirdcamera_yaw;
	float thirdcamera_pitch;
};

class FirstRoleCamera : public Camera
{
public:
	FirstRoleCamera() :Camera() {}
	void InitializeFirstRoleCamera();
	void SetYawAndPitch(float yaw, float pitch);
	void RotateCamera(float xoffset, float yoffset);
	void UpdateForward();
	DirectX::XMMATRIX GetViewMatrix();
	DirectX::XMMATRIX GetViewMatrixForSkybox();

public:
	void SetTarget(DirectX::XMFLOAT3 pos) { firstcamera_position = pos; }
	DirectX::XMFLOAT3 GetPosition() { return firstcamera_position; }
	DirectX::XMFLOAT3 GetForward() { return firstcamera_forward; }
private:
	float firstcamera_pitch;
	float firstcamera_yaw;
	float firstcamera_rotateSpeed;
	DirectX::XMFLOAT3 firstcamera_position;
	DirectX::XMFLOAT3 firstcamera_forward;
	DirectX::XMFLOAT3 firstcamera_up;
};