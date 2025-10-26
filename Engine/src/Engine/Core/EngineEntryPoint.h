#ifndef __ENTRY_H__
#define __ENTRY_H__

#include "EngineApp.h"
#include "Engine/Core/AssetManager.h"
#include "Logging.h"
#include <iostream>

extern Engine::EngineApp* Engine::CreateApplication();

int main(int argc, char** argv)
{
    Engine::Log::Init();
    Engine::AssetManager::Init();
    
    auto app = Engine::CreateApplication();
    try {
        app->Run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    delete app;

    
    return 0;
}

#endif