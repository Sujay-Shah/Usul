#include "PerspectiveCamera.h"

#define GLOBAL_UP_VECTOR glm::vec3(0.0f, 1.0f, 0.0f)

namespace Engine
{
    PerspectiveCamera::PerspectiveCamera()
    {
        m_type = CameraType::Perspective;
        m_data.view = glm::mat4(1.0f);
        m_data.projection = glm::perspective(glm::radians(m_data.fov), m_data.aspectRatio, m_data.nearClip, m_data.farClip);

        m_Yaw = YAW;
        m_Pitch = PITCH;

        Update();
    }

    PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio)
    {
        m_type = CameraType::Perspective;
        m_data.view = glm::mat4(1.0f);

        m_data.fov = fov;
        m_data.aspectRatio = aspectRatio;

        m_data.projection = glm::perspective(glm::radians(m_data.fov), m_data.aspectRatio, -1.0f, 1.0f);

		m_Yaw = YAW;
		m_Pitch = PITCH;

        Update();
    }

    PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio, float nearClip, float farClip)
    {
        m_type = CameraType::Perspective;

        m_data.fov = fov;
        m_data.aspectRatio = aspectRatio;
        m_data.nearClip = nearClip;
        m_data.farClip = farClip;

        m_data.projection = glm::perspective(glm::radians(m_data.fov), m_data.aspectRatio, m_data.nearClip, m_data.farClip);

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
        m_data.view = glm::lookAt(m_Front, glm::vec3(0.0f), GLOBAL_UP_VECTOR);
#else
        front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
        front.y = sin(glm::radians(m_Pitch));
        front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

		m_Front = glm::normalize(front);

		m_Right = glm::normalize(glm::cross(m_Front, GLOBAL_UP_VECTOR));
		m_Up = glm::normalize(glm::cross(m_Right, m_Front));

		m_data.view = glm::lookAt(m_data.pos, m_data.pos + m_Front, m_Up);
#endif        
       
        m_data.viewProjection = m_data.projection * m_data.view;
    }

	void PerspectiveCamera::SetProjection(float fov)
	{
        m_data.fov = fov;
        m_data.projection = glm::perspective(glm::radians(m_data.fov), m_data.aspectRatio, m_data.nearClip, m_data.farClip);
	}

	void PerspectiveCamera::SetRotation(float yaw, float pitch, float roll/*=0.0f*/)
	{
        m_Pitch = pitch;
        m_Yaw = yaw;
	}

}