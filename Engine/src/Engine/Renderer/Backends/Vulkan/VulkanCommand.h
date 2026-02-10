#pragma once

#include <vulkan/vulkan.hpp>
#include "VulkanDevice.h"

namespace Engine {

    class VulkanCommand {
    public:
        VulkanCommand(const std::shared_ptr<VulkanDevice>& device);
        ~VulkanCommand();

        void Create();
        void Cleanup();

        VkCommandBuffer getCommandBuffer() const { return m_commandBuffer; }
        VkSemaphore getImageAvailableSemaphore() const { return m_imageAvailableSemaphore; }
        VkSemaphore getRenderFinishedSemaphore() const { return m_renderFinishedSemaphore; }
        VkFence getInFlightFence() const { return m_inFlightFence; }
        VkCommandPool getCommandPool() const { return m_commandPool; }

    private:
        void createCommandPool();
        void createCommandBuffers();
        void createSyncObjects();

    private:
        std::shared_ptr<VulkanDevice> m_device;
        
        VkCommandPool m_commandPool;
        VkCommandBuffer m_commandBuffer;

        VkSemaphore m_imageAvailableSemaphore;
        VkSemaphore m_renderFinishedSemaphore;
        VkFence m_inFlightFence;
    };
}
