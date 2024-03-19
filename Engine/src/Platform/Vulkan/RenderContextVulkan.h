//
// Created by snsha on 2024-02-01.
//

#ifndef USUL_RENDERCONTEXTVULKAN_H
#define USUL_RENDERCONTEXTVULKAN_H

#include "Renderer/RenderContext.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

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

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData) {

            ENGINE_CORE_WARN("validation layer: {0}",pCallbackData->pMessage);

            return VK_FALSE;
        }

    private:
        GLFWwindow* m_windowHandle = nullptr;

        bool checkValidationLayerSupport();
        std::vector<const char *> getRequiredExtensions();
        void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

        void setupDebugMessenger();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    public:
        VkInstance m_instance;
        VkDebugUtilsMessengerEXT m_debugMessenger;
        VkDeviceCreateInfo m_DeviceCI;
        PhysicalDevices m_PhysicalDevices;
        size_t m_PhysicalDeviceIndex;
        std::vector<const char*> m_validationLayers = {
                "VK_LAYER_KHRONOS_validation"
        };
        const bool m_enableValidationLayers = true;
    };

} // Engine

#endif //USUL_RENDERCONTEXTVULKAN_H
