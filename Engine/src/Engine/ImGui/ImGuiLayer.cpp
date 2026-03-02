#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "ImGuiLayer.h"
#include "imgui.h"

#include "Engine/Core/EngineApp.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Renderer/FrameBuffer.h"
#include "Event/ApplicationEvent.h"
#include "ImGuizmo.h"
#include "RHI/rhi.hpp"

//#define ENGINE_BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

namespace Engine
{
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    ImGuiLayer::ImGuiLayer()
    :
    Layer("ImGuiLayer")
    {}

    ImGuiLayer::~ImGuiLayer()
    {}

    void ImGuiLayer::OnAttach()
    {
#if ENABLE_EXAMPLE & 0
        FramebufferSpecification fbSpec;
        fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
        fbSpec.Width = 1280;
        fbSpec.Height = 720;
        m_Framebuffer = Framebuffer::Create(fbSpec);
#endif

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); 
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable EventCategoryKeyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer bindings
        EngineApp& app = EngineApp::Get();
        GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetWindow());

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer bindings
        rhi::imgui_init();
    }

    void ImGuiLayer::OnDetach()
    {
        rhi::imgui_shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiLayer::OnImGuiRender()
    {
#if ENABLE_EXAMPLE & 0
        if(_showExamples)
        {
            ShowExamples();
        }
#endif
    }

    void ImGuiLayer::Begin()
    {
        // Start the Dear ImGui frame
        rhi::imgui_new_frame();
        ImGui::NewFrame();

        ImGuizmo::BeginFrame();

 #if ENABLE_EXAMPLE && ENABLE_EDITOR_MODE && 0
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Viewport");
        auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
        auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        auto viewportOffset = ImGui::GetWindowPos();
        m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
        m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();

        BlockEvents(!m_ViewportHovered);

        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

        uint64_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
        ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

        ImGui::End();
        ImGui::PopStyleVar();
#endif
    }

    void ImGuiLayer::End()
    {
        ImGuiIO& io = ImGui::GetIO();
        EngineApp& app = EngineApp::Get();
        io.DisplaySize = ImVec2(app.GetWindow().GetWidth(), app.GetWindow().GetHeight());

        // Rendering
        ImGui::Render();
    	
        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }

    void ImGuiLayer::OnUpdate(const Timestep &ts)
    {

    }

    void ImGuiLayer::OnEvent(Event &e)
    {
#if ENABLE_EXAMPLE & 0
        //resize
        WindowResizeEvent * we = dynamic_cast<WindowResizeEvent*>(&e);
        if(we)
        {
            if (FramebufferSpecification spec = m_Framebuffer->GetSpecification();
                    we->GetWidth() > 0.0f && we->GetHeight() > 0.0f && // zero sized framebuffer is invalid
                    (spec.Width != we->GetWidth() || spec.Height != we->GetHeight()))
            {
                m_Framebuffer->Resize((uint32_t)we->GetWidth(), (uint32_t)we->GetHeight());
            }
            //ENGINE_WARN("{0}",we->ToString());
        }
#endif
        if (m_BlockEvents)
        {
            ImGuiIO& io = ImGui::GetIO();
            e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
            e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
        }

    }
#if ENABLE_EXAMPLE & 0
    void ImGuiLayer::BindOrUnbindFrameBuffer(bool val)
    {
        if(val)
        {
            m_Framebuffer->Bind();
            m_Framebuffer->ClearAttachment(1, -1);
        }
        else
        {
            m_Framebuffer->Unbind();
        }
    }

    bool ImGuiLayer::IsViewportFocused() const
    {
        return m_ViewportFocused;
    }

    void ImGuiLayer::AddExample(const std::string& name)
    {
        _Examples.push_back(name.data());
    }

    void ImGuiLayer::ShowExamples()
    {
        if(_Examples.empty())
        {
            return;
        }
        ImGui::Begin("Example Switcher");

        // ImGui combo box to switch between objects
        if (ImGui::BeginCombo("Select Example", _Examples[_currentExampleIndex]))
        {
            for(int i=0;i<_Examples.size();++i)
            {
                bool is_selected = (_Examples[_currentExampleIndex] == _Examples[i]);
                if (ImGui::Selectable(_Examples[i], is_selected))
                    _currentExampleIndex = i;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();



            }
            ImGui::EndCombo();
        }

        // End the ImGui window
        ImGui::End();
    }

     const char* ImGuiLayer::GetCurrentExampleName()
    {
        return _Examples[_currentExampleIndex];
    }

#endif
}
