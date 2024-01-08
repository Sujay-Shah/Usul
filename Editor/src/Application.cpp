#include <Engine.h>
#include <Engine/Core/EngineEntryPoint.h>

#include "TestLayer.h"
#include "Sandbox2D.h"
#include "LightingExample.h"

class Application : public Engine::EngineApp
{
    public:
        Application()
        {
            //Push different examples
            PushLayer(new LightingExample());
            PushLayer(new TestLayer());
            //PushLayer(new Sandbox2D());
        }

        ~Application() {}
}; 

Engine::EngineApp* Engine::CreateApplication()
{
    return new Application();
}