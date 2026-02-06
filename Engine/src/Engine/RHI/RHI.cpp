#include "Engine/RHI/RHI.h"

// Backend implementations
#include "Platform/Dummy/RHIDummy.h"
#include "Platform/OpenGL/RendererAPIOpenGL.h"
#include "Platform/Vulkan/RendererAPIVulkan.h"


namespace Engine
{
    namespace RHI
    {
        static RHIBackend* s_backend = nullptr;

        void Init(RendererAPI::API api)
        {
            switch (api)
            {
                case RendererAPI::API::None:
                    s_backend = new RHIDummy();
                    break;
                case RendererAPI::API::OpenGL:
                    // s_backend = new RendererAPIOpenGL(); // Not implemented yet
                    s_backend = new RHIDummy();
                    break;
                case RendererAPI::API::Vulkan:
                    // s_backend = new RendererAPIVulkan(); // Not implemented yet
                    s_backend = new RHIDummy();
                    break;
            }
            s_backend->Init();
        }

        void Shutdown()
        {
            s_backend->Shutdown();
            delete s_backend;
        }

        Handle CreateBuffer(const BufferUsage usage, const uint32_t size, const void* data)
        {
            return s_backend->CreateBuffer(usage, size, data);
        }

        Handle CreateTexture(const TextureFormat format, const uint32_t width, const uint32_t height, const void* data)
        {
            return s_backend->CreateTexture(format, width, height, data);
        }

        Handle CreateShader(const std::string& filepath)
        {
            return s_backend->CreateShader(filepath);
        }

        Handle CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
        {
            return s_backend->CreateGraphicsPipeline(desc);
        }

        void DestroyBuffer(Handle handle)
        {
            s_backend->DestroyBuffer(handle);
        }

        void DestroyTexture(Handle handle)
        {
            s_backend->DestroyTexture(handle);
        }

        void DestroyShader(Handle handle)
        {
            s_backend->DestroyShader(handle);
        }

        void DestroyGraphicsPipeline(Handle handle)
        {
            s_backend->DestroyGraphicsPipeline(handle);
        }

        void Submit(const CommandList& commandList)
        {
            s_backend->Submit(commandList);
        }

        void Present()
        {
            s_backend->Present();
        }
    }
}
