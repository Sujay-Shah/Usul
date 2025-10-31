#ifndef __EDITORLAYER_H__
#define __EDITORLAYER_H__

#include "Engine.h"

namespace Engine
{
    class Framebuffer;
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
