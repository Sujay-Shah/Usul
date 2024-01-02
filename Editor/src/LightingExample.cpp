#include "LightingExample.h"
#include "imgui.h"
#include <glm/gtc/type_ptr.hpp>

LightingExample::LightingExample() :
	Engine::Layer("LightExample"), m_cameraController(1280.0f / 720.0f), m_cameraPosition(-5.0,0.0f,0.0f)
{
	std::string path = "../Editor/assets/";

	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};

	unsigned int indices[] = {
		// Front face
	0, 1, 2,
	2, 3, 0,

	// Back face
	4, 5, 6,
	6, 7, 4,

	// Left face
	0, 3, 7,
	7, 4, 0,

	// Right face
	1, 5, 6,
	6, 2, 1,

	// Top face
	3, 2, 6,
	6, 7, 3,

	// Bottom face
	0, 1, 5,
	5, 4, 0
	};

	//normal cube
	m_cubeVA = Engine::VertexArray::Create();
	Engine::Ref<Engine::VertexBuffer> cubeVB;
	cubeVB = Engine::VertexBuffer::Create(vertices, sizeof(vertices));
	cubeVB->SetLayout({
		{ Engine::ShaderDataType::Float3, "aPos"},
		{ Engine::ShaderDataType::Float3, "aNormal"}
		});
	m_cubeVA->AddVertexBuffer(cubeVB);

	/*Engine::Ref<Engine::IndexBuffer> cubeIB;
	cubeIB = Engine::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
	m_cubeVA->SetIndexBuffer(cubeIB);*/

	

	//light cube uses same coords, so just set buffers
	m_lightCubeVA = Engine::VertexArray::Create();
	Engine::Ref<Engine::VertexBuffer> lightcubeVB;
	lightcubeVB = Engine::VertexBuffer::Create(vertices, sizeof(vertices));
	lightcubeVB->SetLayout({
		{ Engine::ShaderDataType::Float3, "aPos"},
		{ Engine::ShaderDataType::Float3, "aNormal"}
		});
	m_lightCubeVA->AddVertexBuffer(lightcubeVB);

	/*Engine::Ref<Engine::IndexBuffer> lightcubeIB;
	lightcubeIB = Engine::IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));
	m_lightCubeVA->SetIndexBuffer(lightcubeIB);*/

	m_shaderLibrary.Add(Engine::Shader::Create(path + "shaders/BasicLight.glsl"));
	m_shaderLibrary.Add(Engine::Shader::Create(path+"shaders/CubeLight.glsl"));

	m_cameraController.SetPos(m_cameraPosition);
	m_cameraController.SetRotation(-3.05,5.0,0.0f);

    ENGINE_WARN("Camera Position : {0},{1},{2}",m_cameraPosition.x,m_cameraPosition.y,m_cameraPosition.z);
}

void LightingExample::OnUpdate(const Engine::Timestep& ts)
{
	m_cameraController.OnUpdate(ts);

	Engine::RenderCommand::SetClearColor({ 0.0f, 0.0f, 0.0f, 1 });
	Engine::RenderCommand::Clear();

	Engine::Renderer::BeginScene(m_cameraController.GetCamera());

	
	auto lightShader = m_shaderLibrary.Get("BasicLight");
	lightShader->Bind();
	lightShader->UploadUniformFloat3("objectColor", glm::vec3(1.0f, 0.6f, 0.3f));
	lightShader->UploadUniformFloat3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
	lightShader->UploadUniformFloat3("lightPos", m_lightPos);// glm::vec3(m_lightPos.x, m_lightPos.y, m_lightPos.z));
	lightShader->UploadUniformFloat3("viewPos", m_cameraPosition);
	
	lightShader->UploadUniformMat4("projection", m_cameraController.GetCamera().GetProjectionMatrix());
	lightShader->UploadUniformMat4("view", m_cameraController.GetCamera().GetViewMatrix());
	
	
	lightShader->UploadUniformMat4("model", glm::mat4(1.0f));
	Engine::Renderer::Submit(lightShader, m_cubeVA);

	auto LightcubeShader = m_shaderLibrary.Get("CubeLight");
	LightcubeShader->Bind();
	
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, m_lightPos);
	model = glm::scale(model, glm::vec3(0.5f)); // a smaller cube

	LightcubeShader->UploadUniformMat4("model", model);
	LightcubeShader->UploadUniformMat4("projection", m_cameraController.GetCamera().GetProjectionMatrix());
	LightcubeShader->UploadUniformMat4("view", m_cameraController.GetCamera().GetViewMatrix());
	Engine::Renderer::Submit(LightcubeShader, m_lightCubeVA);

}

void LightingExample::OnImGuiRender()
{
	ImGui::Begin("Setting");
	ImGui::SliderFloat3("Light position", glm::value_ptr(m_lightPos), -10.0f, 10.0f);
	ImGui::End();
}

void LightingExample::OnEvent(Engine::Event& e)
{
	m_cameraController.OnEvent(e);
}

void LightingExample::OnAttach()
{
	//throw std::logic_error("The method or operation is not implemented.");
}

void LightingExample::OnDetach()
{
	//throw std::logic_error("The method or operation is not implemented.");
}

