#ifndef __RENDERERAPI_H__
#define __RENDERERAPI_H__

#include <glm/glm.hpp>

#include "Engine/Core/EngineDefines.h"
#include "VertexArray.h"

namespace Engine
{
    class RendererAPI
    {
        public:
            enum class API
            {
                None = 0,
                OpenGL,
                Vulkan
            };
        public:
            virtual void Init() = 0;
            virtual void SetClearColor(const glm::vec4& color) = 0;
            virtual void Clear() = 0;  
            virtual void DrawIndexed(const Ref<VertexArray>& vertexArray) = 0;
            virtual void DrawArrays(const Ref<VertexArray>& vertexArray) = 0;
            virtual void SetViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

            inline static API GetAPI() { return m_api; }
        private:
            static API m_api;
    };
}

#endif
