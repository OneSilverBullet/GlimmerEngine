#include "camera.h"

using namespace DirectX;

void FirstRoleCamera::InitializeFirstRoleCamera()
{
	XMFLOAT3 target = XMFLOAT3(0.0F, 0.0F, 0.0F);
	firstcamera_position = XMFLOAT3(0.0f, 0.0f, -2500.0f);
	XMStoreFloat3(&firstcamera_forward, XMVector3Normalize(XMLoadFloat3(&target) - XMLoadFloat3(&firstcamera_position)));
	firstcamera_up = XMFLOAT3(0.0f, 1.0f, 0.0f);

	firstcamera_rotateSpeed = 0.3f;
	firstcamera_pitch = 0.0f;
	firstcamera_yaw = 0.0f;
	UpdateForward();
}

void FirstRoleCamera::SetYawAndPitch(float yaw, float pitch)
{
	firstcamera_yaw = yaw;
	firstcamera_pitch = pitch;
	UpdateForward();
}

void FirstRoleCamera::RotateCamera(float xoffset, float yoffset)
{
	firstcamera_pitch += yoffset * firstcamera_rotateSpeed;
	firstcamera_yaw += xoffset * firstcamera_rotateSpeed;
	if (firstcamera_pitch > 89.0f) firstcamera_pitch = 89.0f;
	if (firstcamera_pitch < -89.0f) firstcamera_pitch = -89.0f;
	UpdateForward();
}

void FirstRoleCamera::UpdateForward()
{
	firstcamera_forward.x = cos(XMConvertToRadians(firstcamera_pitch)) * cos(XMConvertToRadians(firstcamera_yaw));
	firstcamera_forward.y = sin(XMConvertToRadians(firstcamera_pitch));
	firstcamera_forward.z = cos(XMConvertToRadians(firstcamera_pitch)) * sin(XMConvertToRadians(firstcamera_yaw));
	XMStoreFloat3(&firstcamera_forward, XMVector3Normalize(XMLoadFloat3(&firstcamera_forward)));
}

XMMATRIX FirstRoleCamera::GetViewMatrix()
{
	XMVECTOR position = XMLoadFloat3(&firstcamera_position);
	XMVECTOR forward = XMLoadFloat3(&firstcamera_forward);
	XMVECTOR up = XMLoadFloat3(&firstcamera_up);
	XMMATRIX viewMatrix = XMMatrixLookToLH(position, forward, up);
	return viewMatrix;
}

XMMATRIX FirstRoleCamera::GetViewMatrixForSkybox()
{
	XMVECTOR position = XMLoadFloat3(&firstcamera_position);
	XMVECTOR forward = XMLoadFloat3(&firstcamera_forward);
	XMVECTOR up = XMLoadFloat3(&firstcamera_up);
	XMMATRIX viewMatrix = XMMatrixLookToLH(position, forward, up);
	XMFLOAT4X4 viewM;
	XMStoreFloat4x4(&viewM, viewMatrix);
	viewM._41 = 0;
	viewM._42 = 0;
	viewM._43 = 0;
	viewM._44 = 1;
	viewMatrix = XMLoadFloat4x4(&viewM);
	return viewMatrix;
}


void ThirdRoleCamera::InitializeThirdRoleCamera()
{
	thirdcamera_target = XMFLOAT3(0.0f, 0.0f, 0.0f);
	thirdcamera_up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	thirdcamera_sensity = 0.3f;
	thirdcamera_distance = 7.0f;
	thirdcamera_yaw = -90.0f;
	thirdcamera_pitch = 0.0f;

}

void ThirdRoleCamera::RotateCamera(float xoffset, float yoffset)
{
	thirdcamera_pitch += yoffset * thirdcamera_sensity;
	thirdcamera_yaw += xoffset * thirdcamera_sensity;
	if (thirdcamera_pitch > 0.0f) thirdcamera_pitch = 0.0f;
	if (thirdcamera_pitch < -89.0f) thirdcamera_pitch = -89.0f;
}

XMFLOAT3 ThirdRoleCamera::GetPosition()
{
	XMFLOAT3 thirdcamera_forward;
	thirdcamera_forward.x = cos(XMConvertToRadians(thirdcamera_pitch)) * cos(XMConvertToRadians(thirdcamera_yaw));
	thirdcamera_forward.y = sin(XMConvertToRadians(thirdcamera_pitch));
	thirdcamera_forward.z = cos(XMConvertToRadians(thirdcamera_pitch)) * sin(XMConvertToRadians(thirdcamera_yaw));
	XMVECTOR forward = XMLoadFloat3(&thirdcamera_forward);
	XMVECTOR target = XMLoadFloat3(&thirdcamera_target);
	XMVECTOR position = target - (forward * thirdcamera_distance);
	XMFLOAT3 pos;
	XMStoreFloat3(&pos, position);
	return pos;
}

XMMATRIX ThirdRoleCamera::GetViewMatrix()
{
	XMFLOAT3 thirdcamera_position = GetPosition();
	XMVECTOR position = XMLoadFloat3(&thirdcamera_position);
	XMVECTOR target = XMLoadFloat3(&thirdcamera_target);
	XMVECTOR up = XMLoadFloat3(&thirdcamera_up);
	XMMATRIX viewMatrix = XMMatrixLookAtLH(position, target, up);
	return viewMatrix;
}

XMMATRIX ThirdRoleCamera::GetViewMatrixForSkybox()
{
	XMFLOAT3 thirdcamera_position = GetPosition();
	XMVECTOR position = XMLoadFloat3(&thirdcamera_position);
	XMVECTOR target = XMLoadFloat3(&thirdcamera_target);
	XMVECTOR up = XMLoadFloat3(&thirdcamera_up);
	XMMATRIX viewMatrix = XMMatrixLookAtLH(position, target, up);
	XMFLOAT4X4 viewM;
	XMStoreFloat4x4(&viewM, viewMatrix);
	viewM._41 = 0;
	viewM._42 = 0;
	viewM._43 = 0;
	viewM._44 = 1;
	viewMatrix = XMLoadFloat4x4(&viewM);
	return viewMatrix;
}
