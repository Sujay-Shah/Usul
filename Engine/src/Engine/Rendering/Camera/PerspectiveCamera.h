#ifndef __PERSPECTIVECAMERA_H__
#define __PERSPECTIVECAMERA_H__

#include "Camera.h"

namespace Engine
{
	const float YAW = -90.0f;
	const float PITCH = 0.0f;
	const float SPEED = 2.5f;
	const float SENSITIVITY = 0.05f;
	const float ZOOM = 45.0f;
#define FIXED_CAM 1
    class PerspectiveCamera : public Camera
    {
        public:
            PerspectiveCamera();
            PerspectiveCamera(float fov, float aspectRatio);
            PerspectiveCamera(float fov, float aspectRatio, float nearClip, float farClip);

            virtual void Update() override;
            //void SetProjection(float fov, float aspectRatio, float nearClip, float farClip);
            void SetProjection(float fov);
            virtual void SetRotation(float yaw, float pitch, float roll=0.0f) override;
    
			//euler angles
			float m_Yaw;
			float m_Pitch;
            glm::vec3 m_Front;
            glm::vec3 m_Up;
            glm::vec3 m_Right;
#if FIXED_CAM
			float m_Radius;
#endif         
    };      

}

#endif