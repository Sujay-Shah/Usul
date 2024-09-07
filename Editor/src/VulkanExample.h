//
// Created by snsha on 2024-02-13.
//

#ifndef USUL_VULKANEXAMPLE_H
#define USUL_VULKANEXAMPLE_H

#include "Engine.h"

class VulkanExample : public Engine::Layer
{
public:
    VulkanExample();
    ~VulkanExample() override;

    void OnUpdate(const Engine::Timestep &ts) override;

    void OnAttach() override;

    void OnDetach() override;

    void OnEvent(Engine::Event &e) override;

    void OnImGuiRender() override;

};


#endif //USUL_VULKANEXAMPLE_H
