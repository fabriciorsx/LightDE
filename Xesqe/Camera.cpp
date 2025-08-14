#include "pch.h"
#include "Camera.h"

using namespace DirectX;

Camera::Camera()
{
    XMStoreFloat4x4(&m_View, XMMatrixIdentity());
    XMStoreFloat4x4(&m_Proj, XMMatrixIdentity());
}

XMMATRIX Camera::GetView() const { return XMLoadFloat4x4(&m_View); }
XMMATRIX Camera::GetProjection() const { return XMLoadFloat4x4(&m_Proj); }

void Camera::SetLens(float fovY, float aspect, float zn, float zf)
{
    XMMATRIX P = XMMatrixPerspectiveFovLH(fovY, aspect, zn, zf);
    XMStoreFloat4x4(&m_Proj, P);
}

void Camera::Walk(float d) { XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(XMVectorReplicate(d), XMLoadFloat3(&m_Look), XMLoadFloat3(&m_Position))); }
void Camera::Strafe(float d) { XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(XMVectorReplicate(d), XMLoadFloat3(&m_Right), XMLoadFloat3(&m_Position))); }
void Camera::Jump(float d) { XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(XMVectorReplicate(d), XMLoadFloat3(&m_Up), XMLoadFloat3(&m_Position))); }

void Camera::Pitch(float angle)
{
    XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&m_Right), angle);
    XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMLoadFloat3(&m_Up), R));
    XMStoreFloat3(&m_Look, XMVector3TransformNormal(XMLoadFloat3(&m_Look), R));
}

void Camera::RotateY(float angle)
{
    XMMATRIX R = XMMatrixRotationY(angle);
    XMStoreFloat3(&m_Right, XMVector3TransformNormal(XMLoadFloat3(&m_Right), R));
    XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMLoadFloat3(&m_Up), R));
    XMStoreFloat3(&m_Look, XMVector3TransformNormal(XMLoadFloat3(&m_Look), R));
}

void Camera::UpdateViewMatrix()
{
    XMVECTOR R = XMLoadFloat3(&m_Right);
    XMVECTOR U = XMLoadFloat3(&m_Up);
    XMVECTOR L = XMLoadFloat3(&m_Look);
    XMVECTOR P = XMLoadFloat3(&m_Position);

    L = XMVector3Normalize(L);
    U = XMVector3Normalize(XMVector3Cross(L, R));
    R = XMVector3Cross(U, L);

    XMStoreFloat3(&m_Right, R);
    XMStoreFloat3(&m_Up, U);
    XMStoreFloat3(&m_Look, L);

    float x = -XMVectorGetX(XMVector3Dot(P, R));
    float y = -XMVectorGetY(XMVector3Dot(P, U));
    float z = -XMVectorGetZ(XMVector3Dot(P, L));

    m_View(0, 0) = m_Right.x; m_View(1, 0) = m_Right.y; m_View(2, 0) = m_Right.z; m_View(3, 0) = x;
    m_View(0, 1) = m_Up.x;    m_View(1, 1) = m_Up.y;    m_View(2, 1) = m_Up.z;    m_View(3, 1) = y;
    m_View(0, 2) = m_Look.x;  m_View(1, 2) = m_Look.y;  m_View(2, 2) = m_Look.z;  m_View(3, 2) = z;
    m_View(0, 3) = 0.0f;     m_View(1, 3) = 0.0f;     m_View(2, 3) = 0.0f;     m_View(3, 3) = 1.0f;
}