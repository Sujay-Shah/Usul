#ifndef __CAMERACONTROLLER_H__
#define __CAMERACONTROLLER_H__

#include "OrthographicCamera.h"
#include "PerspectiveCamera.h"
#include "Event/Event.h"
#include "Event/MouseEvent.h"
#include "Event/WindowEvent.h"
#include "Engine/Core/Timestep.h"

#include <glm/glm.hpp>



namespace Engine
{
    class CameraController
    {
        public:
            CameraController(float aspectRatio);

            Camera& GetCamera() { return m_camera; }
            const Camera& GetCamera() const { return m_camera; }

            void SetPos(const glm::vec3& position);
            void SetRotation(float pitch, float yaw, float roll=0.0f);

            void OnUpdate(const Timestep& ts);

            void OnEvent(Event& e);
        private:
            bool OnScroll(MouseScrolledEvent& e);
            bool OnResize(WindowResizeEvent& e);
        private:
            float m_aspectRatio; 
            float m_zoom = 45.0f;

            //TODO : add ability to handle both cameras
            //OrthographicCamera m_camera;
            PerspectiveCamera m_camera;
            glm::vec3 cameraPos = { 0.0f, 0.0f, 0.0f };

            float cameraMoveSpeed = 5.0f;
            float cameraRotateSpeed = 180.0f;
            float cameraRotation = 0.0f;

            glm::vec2 m_lastCursorPos;
            bool m_first = true; 
    };
}

#endif