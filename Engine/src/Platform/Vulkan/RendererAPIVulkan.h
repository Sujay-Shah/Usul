
#ifndef USUL_RENDERERAPIVULKAN_H
#define USUL_RENDERERAPIVULKAN_H

#include "Renderer/RendererAPI.h"

namespace Engine {

    class RendererAPIVulkan : public RendererAPI
    {
    public:
        void Init() override;

        void SetClearColor(const glm::vec4 &color) override;

        void Clear() override;

        void DrawIndexed(const Ref <VertexArray> &vertexArray) override;

        void DrawArrays(const Ref <VertexArray> &vertexArray) override;

        void SetViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

    };

} // Engine

#endif //USUL_RENDERERAPIVULKAN_H
