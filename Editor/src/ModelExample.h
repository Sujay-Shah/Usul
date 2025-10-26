//
// Created by snsha on 2024-03-15.
//

#ifndef USUL_MODELEXAMPLE_H
#define USUL_MODELEXAMPLE_H

#include "Engine.h"

class ModelExample: public Engine::Layer
{
public:
    ModelExample();
    ~ModelExample() override;

    void OnUpdate(const Engine::Timestep &ts) override;

    void OnAttach() override;

    void OnDetach() override;

    void OnEvent(Engine::Event &e) override;

    void OnImGuiRender() override;

private:
    Engine::ShaderLibrary m_shaderLibrary;

    Engine::CameraController m_cameraController;
    glm::vec3 m_cameraPosition;

    glm::vec3 m_lightPos = glm::vec3(2.2f, 0.0f, 2.0f);

    Engine::Model m_backPack;
};


#endif //USUL_MODELEXAMPLE_H
