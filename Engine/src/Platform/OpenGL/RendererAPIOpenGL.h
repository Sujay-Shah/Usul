#ifndef __RENAPIOPENGL_H__
#define __RENAPIOPENGL_H__

#include "Renderer/RendererAPI.h"

namespace Engine
{
    class RendererAPIOpenGL : public RendererAPI
    {
        public:
            virtual void Init() override;
            virtual void SetClearColor(const glm::vec4& color) override;
            virtual void Clear() override;
            virtual void DrawIndexed(const Ref<VertexArray>& vertexBuffer,  uint32_t indexCount = 0) override;
            virtual void SetViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

            virtual void DrawArrays(const Ref<VertexArray>& vertexArray,uint32_t indexCount=0) override;

    };
}

#endif
