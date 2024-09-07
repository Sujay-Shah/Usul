#ifndef __SANDBOX2D_H__
#define __SANDBOX2D_H__

#include <Engine.h>

class TriangleDemo : public Engine::Layer
{
	public:
		TriangleDemo();
		virtual ~TriangleDemo() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnUpdate(const Engine::Timestep& ts) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Engine::Event& e) override;
	private:
        Engine::ShaderLibrary m_shaderLibrary;
		Engine::CameraController m_cameraController;

		Engine::Ref<Engine::Shader> m_singleColorShader;
        Engine::Ref<Engine::VertexArray> m_triangleVA;

		glm::vec4 triangleColor = {0.3f, 0.2f, 0.8f, 1.0f };

};

#endif
