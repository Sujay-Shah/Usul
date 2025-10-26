//
// Created by snsha on 2024-03-15.
//

#include "ModelExample.h"

ModelExample::~ModelExample() {

}

void ModelExample::OnUpdate(const Engine::Timestep &ts)
{
    m_cameraController.OnUpdate(ts);
    Engine::RenderCommand::SetClearColor({ 0.0f, 0.0f, 0.0f, 1 });
    Engine::RenderCommand::Clear();

    Engine::Renderer::BeginScene(m_cameraController.GetCamera());
    auto modelShader = m_shaderLibrary.Get("Model");
    modelShader->Bind();
    modelShader->UploadUniformMat4("projection", m_cameraController.GetCamera().GetProjectionMatrix());
    modelShader->UploadUniformMat4("view", m_cameraController.GetCamera().GetViewMatrix());

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
    model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));	// it's a bit too big for our scene, so scale it down

    modelShader->UploadUniformMat4("model", model);
    m_backPack.Draw(modelShader);
}

void ModelExample::OnAttach() {

}

void ModelExample::OnDetach() {

}

void ModelExample::OnEvent(Engine::Event &e) {
    m_cameraController.OnEvent(e);
}

void ModelExample::OnImGuiRender() {

}

ModelExample::ModelExample() :
        Engine::Layer("ModelExample"),m_cameraController(1280.0f / 720.0f), m_cameraPosition(-5.0,0.0f,0.0f),
        m_backPack("models/backpack/backpack.obj")
{
    m_shaderLibrary.Add(Engine::Shader::Create("shaders/Model.glsl"));
    m_cameraController.SetPos(m_cameraPosition);
    m_cameraController.SetRotation(-3.05,5.0,0.0f);

}
