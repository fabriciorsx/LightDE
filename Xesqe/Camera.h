#pragma once
#include <DirectXMath.h>

class Camera
{
public:
    Camera();

    DirectX::XMMATRIX GetView() const;
    DirectX::XMMATRIX GetProjection() const;

    DirectX::XMFLOAT3 GetPosition3f() const { return m_Position; }

    void SetLens(float fovY, float aspect, float zn, float zf);
    void UpdateViewMatrix();

    void Walk(float d);
    void Strafe(float d);
    void Jump(float d);
    void Pitch(float angle);
    void RotateY(float angle);


private:
    DirectX::XMFLOAT4X4 m_View = {};
    DirectX::XMFLOAT4X4 m_Proj = {};
    DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, -5.0f };
    DirectX::XMFLOAT3 m_Right = { 1.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 m_Up = { 0.0f, 1.0f, 0.0f };
    DirectX::XMFLOAT3 m_Look = { 0.0f, 0.0f, 1.0f };
};