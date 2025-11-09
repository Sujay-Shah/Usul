#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Engine/Core/Logging.h"

namespace Engine
{
    class Camera
    {
        
	public:
		Camera() = default;
		Camera(const glm::mat4& projection)
			: m_ProjectionMatrix(projection) {}

		virtual ~Camera() = default;

        virtual void Update() {};

        const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
		const glm::vec3& GetPosition() const { return m_Position; }
		void SetPosition(const glm::vec3& position) { m_Position = position; RecalculateViewMatrix(); }
 
		float GetRotation() const { return m_Rotation; }
		void SetRotation(float rotation) { m_Rotation = rotation; RecalculateViewMatrix(); }
        virtual void SetRotation(float yaw, float pitch, float roll=0.0f) {};
        bool IsPerspective() const { return m_type == CameraType::Perspective; }

    protected:
        void RecalculateViewMatrix();

		glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
		glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
		glm::mat4 m_ViewProjectionMatrix = glm::mat4(1.0f);
        glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		float m_Rotation = 0.0f;
        float m_nearClip;
        float m_farClip;

        enum CameraType
        {
            Perspective,
            Orthographic,
            PerspectiveFixed,
            None
        };
        CameraType m_type = CameraType::None;
    };
}

#endif
