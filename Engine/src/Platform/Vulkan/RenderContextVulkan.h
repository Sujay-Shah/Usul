//
// Created by snsha on 2024-02-01.
//

#ifndef USUL_RENDERCONTEXTVULKAN_H
#define USUL_RENDERCONTEXTVULKAN_H

#define GLFW_INCLUDE_VULKAN
#include "Renderer/RenderContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanPipeline.h"
#include "VulkanCommand.h"
#include <memory>

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

    private:
        void drawFrame();
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void createFramebuffers();

    private:
        GLFWwindow* m_windowHandle = nullptr;
        
        VkInstance m_instance;
        VkDebugUtilsMessengerEXT m_debugMessenger;

        std::shared_ptr<VulkanDevice> m_device;
        std::shared_ptr<VulkanSwapChain> m_swapChain;
        std::shared_ptr<VulkanPipeline> m_pipeline;
        std::shared_ptr<VulkanCommand> m_command;
        
        std::vector<VkFramebuffer> m_swapChainFramebuffers;
        
        const std::vector<const char*> m_validationLayers = {
                "VK_LAYER_KHRONOS_validation"
        };
        const bool m_enableValidationLayers = true;

        void setupDebugMessenger();
        bool checkValidationLayerSupport();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        std::vector<const char *> getRequiredExtensions();
        void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    };

} // Engine

#endif //USUL_RENDERCONTEXTVULKAN_H
