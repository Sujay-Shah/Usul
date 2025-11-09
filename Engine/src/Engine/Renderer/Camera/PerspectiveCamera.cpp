#include "PerspectiveCamera.h"

#define GLOBAL_UP_VECTOR glm::vec3(0.0f, 1.0f, 0.0f)

namespace Engine
{
    PerspectiveCamera::PerspectiveCamera()
    {
        m_type = CameraType::Perspective;
        m_ViewMatrix = glm::mat4(1.0f);
        m_ProjectionMatrix = glm::perspective(glm::radians(m_Fov), m_aspectRatio, m_nearClip, m_farClip);

        m_Yaw = YAW;
        m_Pitch = PITCH;

        Update();
    }

    PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio)
    {
        m_type = CameraType::Perspective;
       m_ViewMatrix = glm::mat4(1.0f);

        m_Fov = fov;
        m_aspectRatio = aspectRatio;

        m_ProjectionMatrix = glm::perspective(glm::radians(m_Fov), m_aspectRatio, -1.0f, 1.0f);

		m_Yaw = YAW;
		m_Pitch = PITCH;

        Update();
    }

    PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio, float nearClip, float farClip)
    {
        m_type = CameraType::Perspective;

        m_Fov = fov;
        m_aspectRatio = aspectRatio;
        m_nearClip = nearClip;
        m_farClip = farClip;

        m_ProjectionMatrix = glm::perspective(glm::radians(m_Fov), m_aspectRatio, m_nearClip, m_farClip);

		m_Yaw = YAW;
		m_Pitch = PITCH;

        Update();
        
    }

    void PerspectiveCamera::Update()
    {
        glm::vec3 front;
#if FIXED_CAM
		front.x = m_Radius * cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		front.y = m_Radius * sin(glm::radians(m_Pitch));
		front.z = m_Radius * sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        
        m_Front = front;
       m_ViewMatrix = glm::lookAt(m_Front, glm::vec3(0.0f), GLOBAL_UP_VECTOR);
#else
        front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        front.y = sin(glm::radians(m_Pitch));
        front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

		m_Front = glm::normalize(front);

		m_Right = glm::normalize(glm::cross(m_Front, GLOBAL_UP_VECTOR));
		m_Up = glm::normalize(glm::cross(m_Right, m_Front));

		m_data.view = glm::lookAt(m_data.pos, m_data.pos + m_Front, m_Up);
#endif        
       
        m_ViewProjectionMatrix = m_ProjectionMatrix *m_ViewMatrix;
    }

	void PerspectiveCamera::SetProjection(float fov)
	{
        m_Fov = fov;
        m_ProjectionMatrix = glm::perspective(glm::radians(m_Fov), m_aspectRatio, m_nearClip, m_farClip);
	}

	void PerspectiveCamera::SetRotation(float yaw, float pitch, float roll/*=0.0f*/)
	{
        m_Pitch = pitch;
        m_Yaw = yaw;
        
        Update();
	}

}