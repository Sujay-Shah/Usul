#include "EngineApp.h"
#include "Event/WindowEvent.h"
#include "Input.h"
#include "EngineDefines.h"
#include "Platform/GLFW/TimeGLFW.h"
#include "Timestep.h"
#include "Rendering/Renderer.h"

namespace Engine
{
    EngineApp* EngineApp::m_instance = nullptr;

    EngineApp::EngineApp()
    {   
        ENGINE_ASSERT(!m_instance, "App already exists!");
        m_instance = this;

        m_window = Scope<Window>(Window::Create());
        m_window->SetEventCallback(ENGINE_BIND_EVENT_FN(EngineApp::OnEvent));
        m_window->SetVsync(false);
        
        Renderer::Init();

        m_imguiLayer = new ImGuiLayer();
        PushOverlay(m_imguiLayer);
    }

    void EngineApp::Run()
    {
        while(m_isRunning)
        {
            float time = GetTime();
            Timestep timestep = time - m_lastTime;
            m_lastTime = time;

            if (!m_isMinimized)
            {
                //record draw calls in imgui layer frame buffer to display it in the viewport
                m_imguiLayer->BindOrUnbindFrameBuffer(true);
                for (Layer* layer : m_layerStack)
                {
                    //TODO: refactor this in future
                    if(layer->GetName() == m_imguiLayer->GetCurrentExampleName())
                    {
                        layer->OnUpdate(timestep);
                        break;
                    }
                }
                m_imguiLayer->BindOrUnbindFrameBuffer(false);
            }

            m_imguiLayer->Begin();
            for (Layer* layer : m_layerStack)
            {
                if(layer->GetName() == m_imguiLayer->GetCurrentExampleName())
                {
                    layer->OnImGuiRender();
                    break;
                }
            }
            //TODO: refactor this, currently we need to call imgui layer calls seperately as the LayerStack
            // explicitly contains different examples
            m_imguiLayer->OnImGuiRender();
            m_imguiLayer->End();

            m_window->Update();
        }
    }

    void EngineApp::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(ENGINE_BIND_EVENT_FN(EngineApp::OnWindowCloseEvent));
        dispatcher.Dispatch<WindowResizeEvent>(ENGINE_BIND_EVENT_FN(EngineApp::OnWindowResizeEvent));

        for (auto it = m_layerStack.rbegin(); it != m_layerStack.rend(); ++it)
        {
            (*it)->OnEvent(e);
            if (e.handled)
                break;
        }
    }

    void EngineApp::PushLayer(Layer* layer)
    {
        m_layerStack.PushLayer(layer);
        m_imguiLayer->AddExample(layer->GetName());
        layer->OnAttach();
    }

    void EngineApp::PushOverlay(Layer* layer)
    {
        m_layerStack.PushOverlay(layer);
        layer->OnAttach();
    }

    bool EngineApp::OnWindowCloseEvent(WindowCloseEvent& e)
    {
        m_isRunning = false;
        return true;
    }

    bool EngineApp::OnWindowResizeEvent(WindowResizeEvent& e)
    {
        if (e.GetWidth() == 0 || e.GetHeight() == 0)
        {
            m_isMinimized = true;
            return false;
        }

        m_isMinimized = false;
        Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

        return false;
    }

    void EngineApp::Close()
    {
        m_isRunning = false;
    }
}
