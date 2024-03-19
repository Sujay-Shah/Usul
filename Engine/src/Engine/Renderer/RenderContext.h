#ifndef __RENDERCONTEXT_H__
#define __RENDERCONTEXT_H__

namespace Engine
{
    class RenderContext
    {
        public:
        enum class ExtensionsBit : uint32_t
        {
            NONE = 0x00000000,
#if __APPLE__
            //https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstanceCreateFlagBits.html
            //For MAC_OS
            VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR = 0x00000001
#endif
        };
        //TODO:: pass in creation info in constructor to have a uniform way of constructing render context for diff api
        struct CreateInfo
        {
            std::string		applicationName;
            bool			debugValidationLayers;
            ExtensionsBit	extensions;
            std::string		deviceDebugName;
            void*			pNext;
        };
        struct ResultInfo
        {
            uint32_t		apiVersionMajor;
            uint32_t		apiVersionMinor;
            uint32_t		apiVersionPatch;
            ExtensionsBit	activeExtensions;
            std::string		deviceName;
        };

        const CreateInfo& GetCreateInfo() { return m_CI; }
        const ResultInfo& GetResultInfo() { return m_RI; }

        virtual void Init() = 0;
        virtual void SwapBuffers() = 0;

        virtual void Cleanup() = 0;
        virtual ~RenderContext() = default;
    protected:
        CreateInfo m_CI = {};
        ResultInfo m_RI = {};
    };
}

#endif