#include "ImGuiLayer.h"
#include "imgui.h"
#include "examples/imgui_impl_opengl3.h"
#include "examples/imgui_impl_glfw.h"

#include "Engine/Core/EngineApp.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Rendering/FrameBuffer.h"
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
        FramebufferSpecification fbSpec;
        fbSpec.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
        fbSpec.Width = 1280;
        fbSpec.Height = 720;
        m_Framebuffer = Framebuffer::Create(fbSpec);


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
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File")) {
                // Disabling fullscreen would allow the window to be moved to the front of other windows,
                // which we can't undo at the moment without finer window depth/z control.
                //ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

                /*if (ImGui::MenuItem("Flag: NoSplit", "",
                                    (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))
                    dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
                if (ImGui::MenuItem("Flag: NoResize", "",
                                    (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))
                    dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
                if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "",
                                    (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))
                    dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
                if (ImGui::MenuItem("Flag: PassthruCentralNode", "",
                                    (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))
                    dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
                if (ImGui::MenuItem("Flag: AutoHideTabBar", "",
                                    (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))
                    dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
                ImGui::Separator();*/
                if (ImGui::MenuItem("Exit"))
                {
                    EngineApp::Get().Close();
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
        static bool show = false;
        ImGui::ShowDemoWindow(&show);
        if(_showExamples)
        {
            ShowExamples();
        }
    }

    void ImGuiLayer::Begin()
    {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //setup dockspace
        static bool opt_fullscreen_persistant = true;
        bool opt_fullscreen = opt_fullscreen_persistant;


        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
        // and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &_dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

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
    }

    void ImGuiLayer::End()
    {
        //docking window end
        ImGui::End();

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

    void ImGuiLayer::AddExample(const std::string& name)
    {
        _Examples.push_back(name.data());
    }

    void ImGuiLayer::ShowExamples()
    {
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

    void ImGuiLayer::OnUpdate(const Timestep &ts)
    {


    }

    const char* ImGuiLayer::GetCurrentExampleName()
    {
        return _Examples[_currentExampleIndex];
    }

    void ImGuiLayer::OnEvent(Event &e)
    {
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

        if (m_BlockEvents)
        {
            ImGuiIO& io = ImGui::GetIO();
            e.handled |= e.InCategory(MOUSE) & io.WantCaptureMouse;
            e.handled |= e.InCategory(KEYBOARD) & io.WantCaptureKeyboard;
        }

    }

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

}