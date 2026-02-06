#pragma once

#include "Engine/RHI/RHI.h"

namespace Engine
{
    namespace RHI
    {
        class RHIDummy : public RHIBackend
        {
        public:
            ~RHIDummy() override = default;

            void Init() override;
            void Shutdown() override;

            Handle CreateBuffer(const BufferUsage usage, const uint32_t size, const void* data) override;
            Handle CreateTexture(const TextureFormat format, const uint32_t width, const uint32_t height, const void* data) override;
            Handle CreateShader(const std::string& filepath) override;
            Handle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override;

            void DestroyBuffer(Handle handle) override;
            void DestroyTexture(Handle handle) override;
            void DestroyShader(Handle handle) override;
            void DestroyGraphicsPipeline(Handle handle) override;

            void Submit(const RHI::CommandList& commandList) override;
            void Present() override;
        };
    }
}
