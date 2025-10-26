#include "TextureDemo.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <string>

TextureDemo::TextureDemo()
:
Engine::Layer("Texture Demo"), m_cameraController(1280.0f / 720.0f), m_cameraPosition(0.0f), squareTransform(0.0f)
{
    // square rendering

    m_squareVA = Engine::VertexArray::Create();

    float squareVertices[5 * 4] =
    {
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 0.0f, 1.0f
    };

    Engine::Ref<Engine::VertexBuffer> squareVB;
    squareVB = Engine::VertexBuffer::Create(squareVertices, sizeof(squareVertices));
    squareVB->SetLayout({
        { Engine::ShaderDataType::Float3, "a_Position"},
        { Engine::ShaderDataType::Float2, "a_TexCord"}
    });
    m_squareVA->AddVertexBuffer(squareVB);

    uint32_t squareIndices[6] = { 0, 1, 2, 2, 3, 0 };
    Engine::Ref<Engine::IndexBuffer> squareIB;
    squareIB = Engine::IndexBuffer::Create(squareIndices, sizeof(squareIndices) / sizeof(uint32_t));
    m_squareVA->SetIndexBuffer(squareIB);
    

    /*std::string checkerboard = "textures/Checkerboard.png";
    std::string logo = "textures/ChernoLogo.png";*/
    
    for (int i = 0; i < texturePaths.size(); ++i)
    {
        m_texture.push_back( Engine::Texture2D::Create(texturePaths[i]));
    }
    //m_logo = Engine::Texture2D::Create(path+logo);

    auto shader = m_shaderLibrary.Load("shaders/Texture.glsl");
    shader->Bind();
    shader->UploadUniformInt("u_texture", 0);
}

void TextureDemo::OnUpdate(const Engine::Timestep& ts)
{
    //CLIENT_TRACE("Delta time: {0}", ts.GetTimeSeconds());
    m_cameraController.OnUpdate(ts);

    Engine::RenderCommand::SetClearColor({0.1f, 0.1f, 0.1f, 1});
    Engine::RenderCommand::Clear();

    Engine::Renderer::BeginScene(m_cameraController.GetCamera());

    // triangle
 /*   auto m_shader = m_shaderLibrary.Get("SingleColor");
    m_shader->UploadUniformFloat3("u_color", squareColor);
    Engine::Renderer::Submit(m_shader, m_squareVA);*/


    m_texture[currentTextureIndex]->Bind();
    auto shader = m_shaderLibrary.Get("Texture");
    squareTransform = glm::mat4(1.0f);
    squareTransform = glm::scale(squareTransform,glm::vec3(m_scale));
    Engine::Renderer::Submit(shader, m_squareVA,squareTransform);


    Engine::Renderer::EndScene();
}

void TextureDemo::OnImGuiRender()
{
    ImGui::Begin("Texture");
    if (ImGui::Combo("Texture List", &currentTextureIndex, texturePaths.data(), texturePaths.size())) {
        // This block of code is executed when an item is selected
        // You can put your code here to handle the selection
        ImGui::Text("You selected: %s", texturePaths[currentTextureIndex]);
    }
    ImGui::SliderFloat("Scale", &m_scale, 0.0f, 10.0f);
    ImGui::End();
}

void TextureDemo::OnEvent(Engine::Event& e)
{
    m_cameraController.OnEvent(e);
}
