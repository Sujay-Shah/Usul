#ifndef __TESTLAYER_H__
#define __TESTLAYER_H__

#include <Engine.h>

class TextureDemo : public Engine::Layer
{
    public:
        TextureDemo();

        virtual void OnUpdate(const Engine::Timestep& ts) override;
        virtual void OnImGuiRender() override;
        virtual void OnEvent(Engine::Event& e) override;

    private:
        Engine::ShaderLibrary m_shaderLibrary;
    

        Engine::Ref<Engine::VertexArray> m_squareVA;

        std::vector<Engine::Ref<Engine::Texture2D>> m_texture;

        Engine::CameraController m_cameraController;
        glm::vec3 m_cameraPosition;
        
        glm::mat4 squareTransform;
        float m_scale = 6.5f;

        std::vector<char*> texturePaths{"textures/batman.png","textures/beerus.png","textures/dbz.png","textures/dragon.png"};
        int currentTextureIndex=0;
};

#endif
