#include "ImGuiLayer.h"
#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_glfw.h"

#include "Engine/Core/EngineApp.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Renderer/FrameBuffer.h"
#include "Event/WindowEvent.h"

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
#if ENABLE_EXAMPLE
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
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
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
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 410");
    }

    void ImGuiLayer::OnDetach()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void ImGuiLayer::OnImGuiRender()
    {
#if ENABLE_EXAMPLE
        if(_showExamples)
        {
            ShowExamples();
        }
#endif
    }

    void ImGuiLayer::Begin()
    {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

 #if ENABLE_EXAMPLE
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
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    	
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
#if ENABLE_EXAMPLE
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
            e.handled |= e.InCategory(MOUSE) & io.WantCaptureMouse;
            e.handled |= e.InCategory(KEYBOARD) & io.WantCaptureKeyboard;
        }

    }
#if ENABLE_EXAMPLE
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
