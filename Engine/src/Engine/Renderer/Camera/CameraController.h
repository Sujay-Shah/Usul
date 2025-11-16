#ifndef __CAMERACONTROLLER_H__
#define __CAMERACONTROLLER_H__

#include "OrthographicCamera.h"
#include "PerspectiveCamera.h"
#include "Event/Event.h"
#include "Event/MouseEvent.h"
#include "Event/WindowEvent.h"
#include "Engine/Core/Timestep.h"

#include <glm/glm.hpp>
#include <memory>


namespace Engine
{
    class CameraController
    {
        public:
            CameraController(float aspectRatio, CameraType type = CameraType::Perspective);

            Camera& GetCamera() { return *m_camera; }
            const Camera& GetCamera() const { return *m_camera; }

            void SetPos(const glm::vec3& position);
            void SetRotation(float pitch, float yaw, float roll=0.0f);
            void OnResize(float width, float height);
            void OnUpdate(const Timestep& ts);

            void OnEvent(Event& e);
        private:
            bool OnScroll(MouseScrolledEvent& e);
            bool OnResize(WindowResizeEvent& e);
        private:
            float m_aspectRatio; 
            float m_zoom = 45.0f;

            std::unique_ptr<Camera> m_camera;
            glm::vec3 cameraPos = { 0.0f, 0.0f, 0.0f };

            float cameraMoveSpeed = 5.0f;
            float cameraRotateSpeed = 180.0f;
            float cameraRotation = 0.0f;

            glm::vec2 m_lastCursorPos;
            bool m_first = true; 
    };
}

#endif