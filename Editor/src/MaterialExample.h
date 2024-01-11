//
// Created by snsha on 2024-01-10.
//

#ifndef USUL_MATERIALEXAMPLE_H
#define USUL_MATERIALEXAMPLE_H

#include "Engine.h"
class MaterialExample : public Engine::Layer
{
public:
    MaterialExample();
    ~MaterialExample() override;

    void OnUpdate(const Engine::Timestep &ts) override;

    void OnAttach() override;

    void OnDetach() override;

    void OnEvent(Engine::Event &e) override;

    void OnImGuiRender() override;

private:
    Engine::ShaderLibrary m_shaderLibrary;

    Engine::Ref<Engine::VertexArray> m_cubeVA;
    Engine::Ref<Engine::VertexArray> m_lightCubeVA;

    Engine::CameraController m_cameraController;
    glm::vec3 m_cameraPosition;

    glm::vec3 m_lightPos = glm::vec3(2.2f, 0.0f, 2.0f);

    Engine::Ref<Engine::Material> m_material;
};


#endif //USUL_MATERIALEXAMPLE_H
