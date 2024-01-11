#ifndef LightingExample_h__
#define LightingExample_h__

#include "Engine.h"

class LightingExample : public Engine::Layer
{
public:
	LightingExample();

	virtual void OnUpdate(const Engine::Timestep& ts) override;
	virtual void OnImGuiRender() override;
	virtual void OnEvent(Engine::Event& e) override;
	void OnAttach() override;
	void OnDetach() override;

private:
	Engine::ShaderLibrary m_shaderLibrary;

	Engine::Ref<Engine::VertexArray> m_cubeVA;
	Engine::Ref<Engine::VertexArray> m_lightCubeVA;

	Engine::CameraController m_cameraController;
	glm::vec3 m_cameraPosition;

	glm::vec3 m_lightPos = glm::vec3(2.2f, 0.0f, 2.0f);

};


#endif // LightingExample_h__
