#include <Engine.h>
#include <Engine/Core/EngineEntryPoint.h>

#include "TestLayer.h"
#include "Sandbox2D.h"
#include "LightingExample.h"
#include "MaterialExample.h"
#include "ModelExample.h"
#include "VulkanExample.h"

class Application : public Engine::EngineApp
{
    public:
        Application()
        {
            //Push different example
            //PushLayer(new MaterialExample());
#if API_OPENGL
            //PushLayer(new LightingExample());
            //PushLayer(new TestLayer());
            //PushLayer(new Sandbox2D());
            PushLayer(new ModelExample(_path));
#else
            PushLayer(new VulkanExample());
#endif
        }

        std::string _path = "../Editor/assets/";
        ~Application() {}
}; 

Engine::EngineApp* Engine::CreateApplication()
{
    return new Application();
}