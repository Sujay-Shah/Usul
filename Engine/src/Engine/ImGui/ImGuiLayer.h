#ifndef __IMGUILAYER_H__
#define __IMGUILAYER_H__

#include "Engine/Core/Layer.h"

namespace Engine
{
    class ImGuiLayer : public Layer
    {
        public:
            ImGuiLayer();
            ~ImGuiLayer();

            virtual void OnAttach() override;
            virtual void OnDetach() override;
            virtual void OnImGuiRender() override;

            void Begin();
            void End();
    };
}

#endif
