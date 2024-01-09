#ifndef __IMGUILAYER_H__
#define __IMGUILAYER_H__

#include "Engine/Core/Layer.h"

namespace Engine
{
    class Framebuffer;
    class ImGuiLayer : public Layer
    {
        public:
            ImGuiLayer();
            ~ImGuiLayer();

            virtual void OnAttach() override;
            virtual void OnDetach() override;
            virtual void OnImGuiRender() override;
            void OnUpdate(const Timestep &ts) override;

            void Begin();
            void End();

            void BlockEvents(bool block) { m_BlockEvents = block; }

            void AddExample(const std::string& name);
            void ShowExamples();
            const char* GetCurrentExampleName();

            void OnEvent(Event &e) override;

            void BindOrUnbindFrameBuffer(bool);

            bool IsViewportFocused() const;
    private:
        //This is to run and switch examples on the fly
        std::vector<const char *> _Examples;
        int _currentExampleIndex = 0;
        bool _showExamples = true;
        bool _dockspaceOpen = true;

        //once there are different features in the renderer,
        //this stuff can be ported into a seperate editor layer in future
        Ref<Framebuffer> m_Framebuffer;
        bool m_BlockEvents = true;

        bool m_ViewportFocused = false, m_ViewportHovered = false;
        glm::vec2 m_ViewportBounds[2];
        glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
    };
}

#endif
