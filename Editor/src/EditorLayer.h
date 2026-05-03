#ifndef __EDITORLAYER_H__
#define __EDITORLAYER_H__

#include "Engine.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Engine/Renderer/Camera/EditorCamera.h"
#include "Engine/Renderer/SceneRenderer.h"
#include "RHI/rhi_types.hpp"
namespace Engine
{
    class EditorLayer : public Layer
    {
        public:
            EditorLayer();
            ~EditorLayer();

            virtual void OnAttach() override;
            virtual void OnDetach() override;
            virtual void OnImGuiRender() override;
            void OnUpdate(const Timestep &ts) override;

            void BlockEvents(bool block) { m_BlockEvents = block; }

            void OnEvent(Event &e) override;

            bool IsViewportFocused() const;
    private:
		bool OnKeyPressed(KeyPressedEvent& e);

		void NewScene();
		void OpenScene();
        //void SaveScene();
		void SaveSceneAs();
    private:
        EditorCamera m_EditorCamera;
        CameraController m_CameraController;
        // Render settings exposed to the ImGui "Renderer" panel
        RenderSettings m_RenderSettings;

        // Scene renderer (owns G-Buffer, light/shadow pipelines)
        SceneRenderer m_SceneRenderer;

        bool m_BlockEvents = true;
        bool m_ViewportFocused = false, m_ViewportHovered = false;
        glm::vec2 m_ViewportBounds[2] = {};

        // Raw viewport size tracker for resize detection
        glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
        glm::vec2 m_AllocatedViewportSize = { 0.0f, 0.0f };

        //Temp
        //Ref<VertexArray> m_SquareVA;
		Ref<Shader> m_FlatColorShader;
        Ref<Scene> m_ActiveScene;
		Entity m_SquareEntity;
		Entity m_CameraEntity;

        int m_GizmoType = -1;
        
        //Panels
        SceneHierarchyPanel m_SceneHierarchyPanel;
        
        glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.8f, 1.0f };
    };

}
#endif
