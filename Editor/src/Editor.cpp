#include <Engine.h>
#include <Engine/Core/EngineEntryPoint.h>

#include "EditorLayer.h"
namespace Engine
{
    class Editor : public EngineApp
    {
        public:
            Editor()
            {
                //Push different example
                //PushLayer(new MaterialExample());
                PushLayer(new EditorLayer());
            }
            ~Editor() {}
    }; 

    Engine::EngineApp* Engine::CreateApplication()
    {
        return new Editor();
    }
}