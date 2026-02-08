#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "Engine/Core/EngineDefines.h"

#include "RendererAPI.h"
#include "Shader.h"
#include "VertexArray.h"
#include "Renderer/Camera/Camera.h"
#include "RHI/Texture.h"
#include <glm/glm.hpp>

namespace Engine
{
    class Renderer
    {
        public:
            static void Init();
            static void BeginScene(const Camera& camera);
            static void EndScene();
            static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));
            static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const Ref<Texture2D>& texture, const glm::mat4& transform = glm::mat4(1.0f));

            static void OnWindowResize(uint32_t width, uint32_t height);
            static void SetViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

            inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

            static void SetClearColor(const glm::vec4& color);
            static void Clear();
            static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0);
            static void DrawArrays(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0);

            static void Cleanup();
        private:
            struct SceneData
            {
                glm::mat4 m_viewProjectionMatrix;
            };

            static Scope<SceneData> s_sceneData;
            static Scope<RendererAPI> s_rendererAPI;
    };
}

#endif
