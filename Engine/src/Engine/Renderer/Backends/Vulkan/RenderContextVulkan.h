//
// Created by snsha on 2024-02-01.
//

#ifndef USUL_RENDERCONTEXTVULKAN_H
#define USUL_RENDERCONTEXTVULKAN_H

#include "Renderer/RenderContext.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <optional>

struct GLFWwindow;

namespace Engine
{

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

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
        void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

        void setupDebugMessenger();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        void pickPhysicalDevice();

        bool isDeviceSuitable(VkPhysicalDevice device);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        void createLogicalDevice();
        void createSurface();
        void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void createGraphicsPipeline();
        void createFramebuffers();
        void createCommandPool();
        void createCommandBuffers();
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void createSyncObjects();
        void drawFrame();

        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        VkShaderModule createShaderModule(const std::vector<char>& code);

    public:
        const int MAX_FRAMES_IN_FLIGHT = 2;

        VkInstance m_instance;
        VkDebugUtilsMessengerEXT m_debugMessenger;
        VkDeviceCreateInfo m_DeviceCI;
        PhysicalDevices m_PhysicalDevices;
        size_t m_PhysicalDeviceIndex;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        //logical device
        VkDevice m_logicalDevice;
        VkQueue m_graphicsQueue;
        VkQueue m_presentQueue;
        VkSurfaceKHR m_surface;
        VkSwapchainKHR m_swapChain;
        std::vector<VkImage> m_swapChainImages;
        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;
        std::vector<VkImageView> m_swapChainImageViews;
        std::vector<VkFramebuffer> m_swapChainFramebuffers;

        VkPipelineLayout m_pipelineLayout;
        VkRenderPass m_renderPass;
        VkPipeline m_graphicsPipeline;
        VkCommandPool m_commandPool;
        std::vector<VkCommandBuffer> m_commandBuffers;
        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;
        std::vector<VkFence> m_inFlightFences;
        uint32_t m_currentFrame = 0;

        std::vector<const char*> m_validationLayers = {
                "VK_LAYER_KHRONOS_validation"
        };

        const std::vector<const char*> deviceExtensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
#if __APPLE__
                ,"VK_KHR_portability_subset"
#endif
        };

        const bool m_enableValidationLayers = true;
    };

} // Engine

#endif //USUL_RENDERCONTEXTVULKAN_H
