#include "EngineApp.h"
#include "Event/ApplicationEvent.h"
#include "Input.h"
#include "EngineDefines.h"
#include "Platform/GLFW/TimeGLFW.h"
#include "Timestep.h"
#include "Renderer/Renderer.h"
#include "RHI/rhi.hpp"
#include <GLFW/glfw3.h>

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
        m_layerStack = new LayerStack();

#if API_VULKAN
        // 1. Init RHI Context globally
        if (!rhi::init({
            .backend = rhi::Backend::Vulkan,
            .validation = true,
            .app_name = "Usul Engine"
        }))
        {
            ENGINE_ERROR("Failed to initialize RHI");
        }

        // 2. Create Swapchain
        GLFWwindow* nativeWindow = static_cast<GLFWwindow*>(m_window->GetNativeWindow());
        if (!rhi::swapchain_create({
            .window_handle = nativeWindow,
            .width = m_window->GetWidth(),
            .height = m_window->GetHeight(),
            .image_count = rhi::MAX_FRAMES_IN_FLIGHT,
            .format = rhi::Format::BGRA8_Srgb,
            .vsync = true,
            .window_type = rhi::WindowType::Glfw
        }))
        {
            ENGINE_ERROR("Failed to create swapchain");
        }
#endif

        m_imguiLayer = new ImGuiLayer();
        PushOverlay(m_imguiLayer);
#if !API_VULKAN
        Renderer::Init();
#endif
    }

    EngineApp::~EngineApp()
    {
        delete m_layerStack;
        m_window.reset();
        rhi::swapchain_destroy();
        rhi::shutdown();
    }

    void EngineApp::Run()
    {
        while(m_isRunning)
        {
            float time = GetTime();
            Timestep timestep = time - m_lastTime;
            m_lastTime = time;
#if !API_VULKAN
            Renderer::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
            Renderer::Clear();

            if (!m_isMinimized)
            {
            #if ENABLE_EXAMPLE & ENABLE_EDITOR_MODE
                //record draw calls in imgui layer frame buffer to display it in the viewport
                m_imguiLayer->BindOrUnbindFrameBuffer(true);
            #endif
                for (Layer* layer : *m_layerStack)
                {
                    //TODO: refactor this in future
                    #if ENABLE_EXAMPLE
                        if(layer->GetName() == m_imguiLayer->GetCurrentExampleName())
                    #endif
                    {
                        layer->OnUpdate(timestep);
                        break;
                    }
                }
            #if ENABLE_EXAMPLE && ENABLE_EDITOR_MODE
                m_imguiLayer->BindOrUnbindFrameBuffer(false);
            #endif
            }

            m_imguiLayer->Begin();
            for (Layer* layer : *m_layerStack)
            {
                #if ENABLE_EXAMPLE
                if(layer->GetName() == m_imguiLayer->GetCurrentExampleName())
                {
                    layer->OnImGuiRender();
                    break;
                }
                #else
                layer->OnImGuiRender();
                #endif
            } 
            //TODO: refactor this, currently we need to call imgui layer calls seperately as the LayerStack
            // explicitly contains different examples
            
            
           
#endif
                m_imguiLayer->Begin();
                for (Layer* layer : *m_layerStack)
                {
                    layer->OnImGuiRender();
                }
                m_imguiLayer->OnImGuiRender();
                m_imguiLayer->End();

                for (Layer* layer : *m_layerStack)
                {
                    layer->OnUpdate(timestep);
                }
                m_window->Update();
        }
    }

    void EngineApp::OnEvent(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(ENGINE_BIND_EVENT_FN(EngineApp::OnWindowCloseEvent));
        dispatcher.Dispatch<WindowResizeEvent>(ENGINE_BIND_EVENT_FN(EngineApp::OnWindowResizeEvent));

        for (auto it = m_layerStack->rbegin(); it != m_layerStack->rend(); ++it)
        {
            (*it)->OnEvent(e);
            if (e.Handled)
                break;
        }
    }

    void EngineApp::PushLayer(Layer* layer)
    {
        m_layerStack->PushLayer(layer);
#if !API_VULKAN && ENABLE_EXAMPLE
        m_imguiLayer->AddExample(layer->GetName());
#endif
        layer->OnAttach();
    }

    void EngineApp::PushOverlay(Layer* layer)
    {
        m_layerStack->PushOverlay(layer);
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

    ImGuiLayer *EngineApp::GetImGuiLayer() const
    {
        return m_imguiLayer;
    }
}
