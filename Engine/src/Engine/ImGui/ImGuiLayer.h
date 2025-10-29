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

            void OnEvent(Event &e) override;
#if ENABLE_EXAMPLE
            void BindOrUnbindFrameBuffer(bool);

            bool IsViewportFocused() const;

            void AddExample(const std::string& name);
            void ShowExamples();
            const char* GetCurrentExampleName();
    private:
        //This is to run and switch examples on the fly
        std::vector<const char *> _Examples;
        int _currentExampleIndex = 0;
        bool _showExamples = true;

        bool m_ViewportFocused = false, m_ViewportHovered = false;
        glm::vec2 m_ViewportBounds[2];
        glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
        Ref<Framebuffer> m_Framebuffer;
    
#endif
    private:
        bool _dockspaceOpen = true;
        bool m_BlockEvents = true;
    };
}

#endif
