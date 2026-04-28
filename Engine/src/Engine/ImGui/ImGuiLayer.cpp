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
    
    // RHI Resources
    static rhi::Texture s_ColorTex = {};
    static rhi::Texture s_DepthTex = {};
    static rhi::CmdBuf  s_Cmd      = {};
    static rhi::Fence   s_Fence    = {};
    static uint32_t     s_ViewportWidth = 1280;
    static uint32_t     s_ViewportHeight = 720;

    ImGuiLayer::ImGuiLayer()
    :
    Layer("ImGuiLayer")
    {}

    ImGuiLayer::~ImGuiLayer()
    {}

    void ImGuiLayer::OnAttach()
    {
#if ENABLE_EXAMPLE
        s_ViewportWidth = 1280;
        s_ViewportHeight = 720;

        s_ColorTex = rhi::texture_create({
            .width = s_ViewportWidth,
            .height = s_ViewportHeight,
            .format = rhi::Format::BGRA8_Srgb,
            .usage = rhi::TextureUsage::ColorTarget | rhi::TextureUsage::Sampled,
            .name = "ViewportColor"
        });

        s_DepthTex = rhi::texture_create({
            .width = s_ViewportWidth,
            .height = s_ViewportHeight,
            .format = rhi::Format::D32_Float,
            .usage = rhi::TextureUsage::DepthTarget,
            .name = "ViewportDepth"
        });
        
        s_Cmd = rhi::cmdbuf_create(0);
        s_Fence = rhi::fence_create(false);
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
#if ENABLE_EXAMPLE
        rhi::texture_destroy(s_ColorTex);
        rhi::texture_destroy(s_DepthTex);
        rhi::cmdbuf_destroy(s_Cmd);
        rhi::fence_destroy(s_Fence);
#endif
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
        rhi::imgui_new_frame();
        ImGui::NewFrame();

        ImGuizmo::BeginFrame();

 #if ENABLE_EXAMPLE && ENABLE_EDITOR_MODE
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

        // Note: Casting rhi::Texture handle to ImTextureID. Backend must support this or use a descriptor set.
        ImGui::Image((ImTextureID)(uintptr_t)s_ColorTex.id, ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

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
#if ENABLE_EXAMPLE
        //resize
        WindowResizeEvent * we = dynamic_cast<WindowResizeEvent*>(&e);
        if(we)
        {
            uint32_t width = (uint32_t)we->GetWidth();
            uint32_t height = (uint32_t)we->GetHeight();
            if (width > 0 && height > 0 && (width != s_ViewportWidth || height != s_ViewportHeight))
            {
                s_ViewportWidth = width;
                s_ViewportHeight = height;

                rhi::texture_destroy(s_ColorTex);
                rhi::texture_destroy(s_DepthTex);

                s_ColorTex = rhi::texture_create({
                    .width = s_ViewportWidth,
                    .height = s_ViewportHeight,
                    .format = rhi::Format::BGRA8_Srgb,
                    .usage = rhi::TextureUsage::ColorTarget | rhi::TextureUsage::Sampled,
                    .name = "ViewportColor"
                });

                s_DepthTex = rhi::texture_create({
                    .width = s_ViewportWidth,
                    .height = s_ViewportHeight,
                    .format = rhi::Format::D32_Float,
                    .usage = rhi::TextureUsage::DepthTarget,
                    .name = "ViewportDepth"
                });
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
#if ENABLE_EXAMPLE 
    void ImGuiLayer::BindOrUnbindFrameBuffer(bool val)
    {
        if(val)
        {
            rhi::cmdbuf_reset(s_Cmd);
            rhi::cmdbuf_begin(s_Cmd);
            rhi::begin_render_pass(s_Cmd, {
                .color = {{ .texture = s_ColorTex, .load_op = rhi::LoadOp::Clear, .store_op = rhi::StoreOp::Store, .clear = {0.1f, 0.1f, 0.1f, 1.0f} }},
                .color_count = 1,
                .depth = { .texture = s_DepthTex, .load_op = rhi::LoadOp::Clear, .store_op = rhi::StoreOp::Store, .clear = {1.0f, 0} },
                .has_depth = true
            });
        }
        else
        {
            rhi::end_render_pass(s_Cmd);
            rhi::cmdbuf_end(s_Cmd);
            rhi::queue_submit(&s_Cmd, 1, nullptr, 0, nullptr, 0, s_Fence);
            rhi::fence_wait(s_Fence);
            rhi::fence_reset(s_Fence);
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
        return "";//_Examples[_currentExampleIndex];
    }

#endif
}
