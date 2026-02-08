#pragma once

#include "RHICommon.h"
#include "RHICommands.h"
#include "Engine/RHI/RendererAPI.h"

#include <vector>
#include <string>

namespace Engine
{
    class Shader; // Forward declare to avoid including Shader.h here

    namespace RHI
    {
        class RHIBackend;

        struct GraphicsPipelineDesc
        {
            Handle shader;
            PrimitiveTopology topology = PrimitiveTopology::Triangles;
            // Add other pipeline states here, e.g. depth/stencil, blend, rasterizer
        };

        // The main RHI class. This is a static class that provides the public API.
        // For now, a command list is just a vector of command structs.
        // A real implementation would be more complex.
        using CommandList = std::vector<CommandBase*>;

        // --- Lifecycle ---
        void Init(RendererAPI::API api);
        void Shutdown();

        // --- Resource Creation ---
        Handle CreateBuffer(const BufferUsage usage, const uint32_t size, const void* data = nullptr);
        Handle CreateTexture(const TextureFormat format, const uint32_t width, const uint32_t height, const void* data = nullptr);
        Handle CreateShader(const std::string& filepath);
        Handle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc);

        // --- Resource Destruction ---
        void DestroyBuffer(Handle handle);
        void DestroyTexture(Handle handle);
        void DestroyShader(Handle handle);
        void DestroyGraphicsPipeline(Handle handle);

        // --- Command Submission ---
        void Submit(const CommandList& commandList);
        void Present();

        // The interface for a RHI backend (Vulkan, OpenGL, etc.)
        class RHIBackend
        {
        public:
            virtual ~RHIBackend() = default;

            virtual void Init() = 0;
            virtual void Shutdown() = 0;

            virtual Handle CreateBuffer(const BufferUsage usage, const uint32_t size, const void* data) = 0;
            virtual Handle CreateTexture(const TextureFormat format, const uint32_t width, const uint32_t height, const void* data) = 0;
            virtual Handle CreateShader(const std::string& filepath) = 0;
            virtual Handle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) = 0;

            virtual void DestroyBuffer(Handle handle) = 0;
            virtual void DestroyTexture(Handle handle) = 0;
            virtual void DestroyShader(Handle handle) = 0;
            virtual void DestroyGraphicsPipeline(Handle handle) = 0;

            virtual void Submit(const CommandList& commandList) = 0;
            virtual void Present() = 0;
        };
    }
}
