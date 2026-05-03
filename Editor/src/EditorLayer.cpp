#include "EditorLayer.h"
#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_glfw.h"

#include "Engine/Core/EngineApp.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "RHI/rhi.hpp"
#include "Event/ApplicationEvent.h"
#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Utils/PlatformUtils.h"

namespace Engine
{
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    EditorLayer::EditorLayer()
    :
    Layer("Editor"), m_CameraController(1280.0f / 720.0f), m_SquareColor({ 0.2f, 0.3f, 0.8f, 1.0f })
    {}

    EditorLayer::~EditorLayer()
    {}

    void EditorLayer::OnAttach()
    {
        m_ActiveScene = CreateRef<Scene>();
        m_SceneHierarchyPanel.SetContext(m_ActiveScene);
        m_SceneRenderer.Init(1280, 720);
    }

    void EditorLayer::OnDetach()
    {
        m_SceneRenderer.Shutdown();
    }

    void EditorLayer::OnImGuiRender()
    {
        // Note: Switch this to true to enable dockspace
		static bool dockspaceOpen = true;
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

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

		// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
		// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
		// all active windows docked into it will lose their parent and become undocked.
		// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
		// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		style.WindowMinSize.x = minWinSizeX;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
					OpenScene();

				ImGui::Separator();

				if (ImGui::MenuItem("New Scene", "Ctrl+N"))
					NewScene();

				/*if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
					SaveScene();*/

				if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
					SaveSceneAs();

				ImGui::Separator();

				if (ImGui::MenuItem("Exit"))
					EngineApp::Get().Close();
				
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Script"))
			{
				if (ImGui::MenuItem("Reload assembly", "Ctrl+R"))
				{
					//ScriptEngine::ReloadAssembly();
				}
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}
        static bool show = true;
        ImGui::End();
        ImGui::ShowDemoWindow(&show);

		m_SceneHierarchyPanel.OnImGuiRender();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport");

		bool viewportFocused = ImGui::IsWindowFocused();
		bool viewportHovered = ImGui::IsWindowHovered();
		EngineApp::Get().GetImGuiLayer()->BlockEvents(!viewportFocused || !viewportHovered);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		// Display the final lit scene output from SceneRenderer
		rhi::Texture colorOut = m_SceneRenderer.GetColorOutput();
		ImGui::Image((ImTextureID)(uintptr_t)colorOut.id, ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		ImGui::End();
		ImGui::PopStyleVar();

		// ---- Renderer Settings panel ----
		ImGui::Begin("Renderer Settings");
		ImGui::Checkbox("Shadows",   &m_RenderSettings.EnableShadows);
		ImGui::Checkbox("Wireframe", &m_RenderSettings.EnableWireframe);
		ImGui::End();
    }

    void EditorLayer::OnUpdate(const Timestep &ts)
    {
		// Resize G-Buffer when viewport changes size
		if (m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f &&
			(m_AllocatedViewportSize.x != m_ViewportSize.x || m_AllocatedViewportSize.y != m_ViewportSize.y))
		{
			m_AllocatedViewportSize = m_ViewportSize;
			m_SceneRenderer.Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_CameraController.OnResize(m_ViewportSize.x, m_ViewportSize.y);
			m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		}

		if (m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f)
			m_SceneRenderer.RenderScene(m_ActiveScene, m_EditorCamera, m_RenderSettings);
    }

    void EditorLayer::OnEvent(Event &e)
    {
		m_CameraController.OnEvent(e);
		
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>(ENGINE_BIND_EVENT_FN(EditorLayer::OnKeyPressed));

        //resize
        /*WindowResizeEvent * we = dynamic_cast<WindowResizeEvent*>(&e);
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
        }*/

    }

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		// Shortcuts
		if (e.IsRepeat())
			return false;

		bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
		bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
		switch (e.GetKeyCode())
		{
			case Key::N:
			{
				if (control)
					NewScene();

				break;
			}
			case Key::O:
			{
				if (control)
					OpenScene();

				break;
			}
			case Key::S:
			{
				if (control && shift)
					SaveSceneAs();

				break;
			}
		}
		return true;
	}

	void EditorLayer::NewScene()
	{
		m_ActiveScene = CreateRef<Scene>();
		m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
		m_SceneHierarchyPanel.SetContext(m_ActiveScene);
	}

	void EditorLayer::OpenScene()
	{
		// Combine the extensions into one pattern using a semicolon
		std::string filepath = FileDialogs::OpenFile("Scene (*.yaml, *.yml)\0*.yaml;*.yml\0");
		if (!filepath.empty())
		{
			m_ActiveScene = CreateRef<Scene>();
			m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_SceneHierarchyPanel.SetContext(m_ActiveScene);

			SceneSerializer serializer(m_ActiveScene);
			serializer.Deserialize(filepath);
		}
	}

	void EditorLayer::SaveSceneAs()
	{
		// Combine the extensions into one pattern using a semicolon
		std::string filepath = FileDialogs::SaveFile("Scene (*.yaml, *.yml)\0*.yaml;*.yml\0");
		if (!filepath.empty())
		{
			SceneSerializer serializer(m_ActiveScene);
			serializer.Serialize(filepath);
		}
	}

    bool EditorLayer::IsViewportFocused() const
    {
        return m_ViewportFocused;
    }
}
