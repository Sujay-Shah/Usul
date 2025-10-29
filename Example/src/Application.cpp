#include <Engine.h>
#include <Engine/Core/EngineEntryPoint.h>

#include "TextureDemo.h"
#include "TriangleDemo.h"
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
            PushLayer(new LightingExample());
            PushLayer(new TextureDemo());
            PushLayer(new TriangleDemo());
            //PushLayer(new ModelExample());
#else
            PushLayer(new VulkanExample());
#endif
        }
        ~Application() {}
}; 

Engine::EngineApp* Engine::CreateApplication()
{
    return new Application();
}