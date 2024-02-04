//
// Created by snsha on 2024-02-01.
//

#ifndef USUL_RENDERCONTEXTVULKAN_H
#define USUL_RENDERCONTEXTVULKAN_H

#include "Renderer/RenderContext.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

struct GLFWwindow;

namespace Engine
{

    class RenderContextVulkan final : public RenderContext
    {
    public:
        RenderContextVulkan(GLFWwindow* windowHandle);

        virtual void Init() override;
        virtual void SwapBuffers() override;

        void Cleanup() override;

        struct PhysicalDevices
        {
            struct PhysicalDeviceInfo
            {
                VkPhysicalDevice m_PhysicalDevice;
                VkPhysicalDeviceFeatures m_Features;
                VkPhysicalDeviceProperties m_Properties;
                VkPhysicalDeviceMemoryProperties m_MemoryProperties;
            };
            std::vector<PhysicalDeviceInfo> m_PDIs;

            PhysicalDevices(const VkInstance& instance);
            PhysicalDevices() {};
            PhysicalDevices(const PhysicalDevices& physicalDevice)
            {
                for (const PhysicalDeviceInfo& physicalDeviceInfo : physicalDevice.m_PDIs)
                {
                    m_PDIs.push_back(physicalDeviceInfo);
                }
            };

           // void FillOutFeaturesAndProperties(Context* pContext);
        };
    private:
        GLFWwindow* m_windowHandle = nullptr;

    public:
        VkInstance m_instance;
        VkDeviceCreateInfo m_DeviceCI;
        PhysicalDevices m_PhysicalDevices;
        size_t m_PhysicalDeviceIndex;

    };

} // Engine

#endif //USUL_RENDERCONTEXTVULKAN_H
