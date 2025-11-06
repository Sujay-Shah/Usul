#include "CameraController.h"

#include "Engine/Core/EngineDefines.h"
#include "Engine/Core/Input.h"
#include "Event/KeyCodes.h"
#include "Event/MouseCodes.h"
#include "Engine/Core/EngineApp.h"

namespace Engine
{
    CameraController::CameraController(float aspectRatio, CameraType type)
        :
        m_aspectRatio(aspectRatio)
    {
        if (type == CameraType::Perspective)
        {
            m_camera = std::make_unique<PerspectiveCamera>(m_zoom, aspectRatio, 0.1f, 100.0f);
        }
        else
        {
            m_camera = std::make_unique<OrthographicCamera>(-m_aspectRatio * m_zoom, m_aspectRatio * m_zoom, -m_zoom, m_zoom);
        }
    }

    void CameraController::SetPos(const glm::vec3& position)
    {
        m_camera->SetPos(position);
    }
    
    void CameraController::OnResize(float width, float height)
	{
		m_aspectRatio = width / height;
		if (m_camera->IsPerspective())
		{
            PerspectiveCamera* camera = static_cast<PerspectiveCamera*>(m_camera.get());
            camera->SetProjection(m_zoom);
		}
        else
        {
            auto* camera = static_cast<OrthographicCamera*>(m_camera.get());
            camera->SetProjection(-m_aspectRatio * m_zoom, m_aspectRatio * m_zoom, -m_zoom, m_zoom);
        }
	}
    
    void CameraController::SetRotation(float pitch, float yaw, float roll)
    {
        m_camera->SetRotation(pitch, yaw, roll);
    }

    void CameraController::OnUpdate(const Timestep& ts)
    {
        EngineApp& app = EngineApp::Get();
        if(app.GetImGuiLayer() 
 #if ENABLE_EXAMPLE    
        && !app.GetImGuiLayer()->IsViewportFocused()
#endif 
    )
        {
            return;
        }
        float deltaTime = ts.GetTimeSeconds();
        float velocity = cameraMoveSpeed * deltaTime;
        glm::vec3 cameraPos = m_camera->GetPos();

#if !FIXED_CAM
        if (Input::IsKeyPressed(KEY_W) || Input::IsKeyPressed(KEY_UP))
        {
            // Assuming m_Front is in the base Camera class
            cameraPos += m_camera->GetFront() * velocity;
        }
        else if (Input::IsKeyPressed(KEY_S) || Input::IsKeyPressed(KEY_DOWN))
        {
            cameraPos -= m_camera->GetFront() * velocity;
        }

        if (Input::IsKeyPressed(KEY_A) || Input::IsKeyPressed(KEY_LEFT))
        {
            cameraPos -= m_camera->GetRight() * velocity;
        }
        else if (Input::IsKeyPressed(KEY_D) || Input::IsKeyPressed(KEY_RIGHT))
        {
            cameraPos += m_camera->GetRight() * velocity;
        }

        if (Input::IsKeyPressed(KEY_Q))
        {
            cameraRotation -= cameraRotateSpeed * ts.GetTimeSeconds();
        }
        else if (Input::IsKeyPressed(KEY_E))
        {
            cameraRotation += cameraRotateSpeed * ts.GetTimeSeconds();
        }
        
        if (Input::IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
        {
			std::pair<float, float> pos = Input::GetMousePos();
            //ENGINE_CORE_WARN("x : {0}, y : {1} ",pos.first,pos.second);
			
			if (m_first)
			{
                m_lastCursorPos = pos;
                m_first = false;
			}
            //TODO: fix camera jumps and replace magic numbers
			float xoffset = glm::clamp(pos.first - m_lastCursorPos.first,-50.0f,50.0f);
			float yoffset = glm::clamp(m_lastCursorPos.second - pos.second, -50.0f, 50.0f); // reversed since y-coordinates go from bottom to top

            //ENGINE_CORE_WARN("xoffset : {0}, yoffset : {1} ", xoffset, yoffset);
            m_lastCursorPos = pos;

            xoffset *= SENSITIVITY;
            yoffset *= SENSITIVITY;

            // You must cast to access derived members
            if (m_camera->IsPerspective())
            {
                PerspectiveCamera* camera = static_cast<PerspectiveCamera*>(m_camera.get());
                camera->m_Yaw += xoffset;
                camera->m_Pitch += yoffset;

                if (camera->m_Pitch > 89.0f)
                {
                    camera->m_Pitch = 89.0f;
                }
                if (camera->m_Pitch < -89.0f)
                {
                    camera->m_Pitch = -89.0f;
                }
            }
        }
		m_camera->SetPos(cameraPos);

#else
        if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
        {
            glm::vec2 pos = Input::GetMousePosition();

            float xoffset = glm::clamp(SENSITIVITY * (pos.x - m_lastCursorPos.x),-1.0f,1.0f);
            float yoffset = glm::clamp(SENSITIVITY * (m_lastCursorPos.y - pos.y),-1.0f,1.0f); // reversed since y-coordinates go from bottom to top

            // Assuming m_Radius is specific to a camera type, you must cast
            PerspectiveCamera* camera = static_cast<PerspectiveCamera*>(m_camera.get()); // Change if it's Orthographic
            camera->m_Radius += xoffset - yoffset;
            //ENGINE_WARN("Radius : {0}", camera->m_Radius);
            camera->m_Radius = glm::clamp(camera->m_Radius, 5.0f, 20.0f);
			ENGINE_CORE_WARN("xoffset : {0}, yoffset : {1} ", xoffset, yoffset);
			m_lastCursorPos = pos;
        }
        else if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
        {
			glm::vec2 pos = Input::GetMousePosition();

            //we clamp the offset values between -1,1 to prevent sudden jumps due to large old and new pos diff
            float xoffset = glm::clamp(SENSITIVITY * (pos.x - m_lastCursorPos.x),-1.0f,1.0f);
            float yoffset = glm::clamp(SENSITIVITY * (m_lastCursorPos.y - pos.y),-1.0f,1.0f);

            //if(xoffset != 0 || yoffset!= 0) ENGINE_CORE_WARN("xoffset : {0}, yoffset : {1} ", xoffset, yoffset);

            auto* camera = static_cast<PerspectiveCamera*>(m_camera.get()); // Change if it's Orthographic
			camera->m_Yaw += xoffset;
			camera->m_Pitch += yoffset;

			if (camera->m_Pitch > 89.0f)
			{
				camera->m_Pitch = 89.0f;
			}
			if (camera->m_Pitch < -89.0f)
			{
				camera->m_Pitch = -89.0f;
			}
            m_lastCursorPos = pos;
        }
        m_camera->Update();
#endif        

        //m_camera->SetRotation(cameraRotation);
        //m_camera->Update();
    }

    void CameraController::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);

        dispatcher.Dispatch<MouseScrolledEvent>(ENGINE_BIND_EVENT_FN(CameraController::OnScroll));
        dispatcher.Dispatch<WindowResizeEvent>(ENGINE_BIND_EVENT_FN(CameraController::OnResize));
    }

    bool CameraController::OnScroll(MouseScrolledEvent& e)
    {
        m_zoom -= e.GetYDiff() * 0.25f;
        m_zoom = (glm::max)(m_zoom, 0.25f);

        if (m_camera->IsPerspective())
        {
            PerspectiveCamera* camera = static_cast<PerspectiveCamera*>(m_camera.get());
            camera->SetProjection(m_zoom);
        }
        else
        {
            OrthographicCamera* camera = static_cast<OrthographicCamera*>(m_camera.get());
            camera->SetProjection(-m_aspectRatio * m_zoom, m_aspectRatio * m_zoom, -m_zoom, m_zoom);
        }
        
        return false;
    }

    bool CameraController::OnResize(WindowResizeEvent& e)
    {
        m_aspectRatio = (float)e.GetWidth() / (float)e.GetHeight();
		if (m_camera->IsPerspective())
		{
            PerspectiveCamera* camera = static_cast<PerspectiveCamera*>(m_camera.get());
            camera->SetProjection(m_zoom);
		}
        else
        {
            OrthographicCamera* camera = static_cast<OrthographicCamera*>(m_camera.get());
            camera->SetProjection(-m_aspectRatio * m_zoom, m_aspectRatio * m_zoom, -m_zoom, m_zoom);
        }
        
        return false;
    }
}