#include "OrthographicCamera.h"
#include "Engine/Core/Logging.h"
namespace Engine
{
    OrthographicCamera::OrthographicCamera()
    {
        m_type = CameraType::Orthographic;
        m_ViewMatrix = glm::mat4(1.0f);
        m_ProjectionMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

    OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
    {
        m_type = CameraType::Orthographic;
        m_ViewMatrix = glm::mat4(1.0f);
        m_ProjectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);

        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

    OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top, float nearClip, float farClip)
    {
        m_type = CameraType::Orthographic;
        m_ViewMatrix = glm::mat4(1.0f);

        m_nearClip = nearClip;
        m_farClip = farClip;

        m_ProjectionMatrix = glm::ortho(left, right, bottom, top, m_nearClip, m_farClip);

        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

    void OrthographicCamera::SetProjection(float left, float right, float bottom, float top)
    {
        m_ProjectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

    void OrthographicCamera::Update()
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position);
        transform *= glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), glm::vec3(0, 0, 1));

        m_ViewMatrix = glm::inverse(transform);
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }
};