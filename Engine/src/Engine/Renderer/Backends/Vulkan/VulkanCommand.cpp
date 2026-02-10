#include "EnginePCH.h"
#include "VulkanCommand.h"
#include <stdexcept>

namespace Engine {

    VulkanCommand::VulkanCommand(const std::shared_ptr<VulkanDevice>& device)
        : m_device(device) {
    }

    VulkanCommand::~VulkanCommand() {
        Cleanup();
    }
    
    void VulkanCommand::Create()
    {
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }

    void VulkanCommand::Cleanup() {
        if (m_imageAvailableSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device->getLogicalDevice(), m_imageAvailableSemaphore, nullptr);
            m_imageAvailableSemaphore = VK_NULL_HANDLE;
        }
        if (m_renderFinishedSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device->getLogicalDevice(), m_renderFinishedSemaphore, nullptr);
            m_renderFinishedSemaphore = VK_NULL_HANDLE;
        }
        if (m_inFlightFence != VK_NULL_HANDLE) {
            vkDestroyFence(m_device->getLogicalDevice(), m_inFlightFence, nullptr);
            m_inFlightFence = VK_NULL_HANDLE;
        }
        if (m_commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_device->getLogicalDevice(), m_commandPool, nullptr);
            m_commandPool = VK_NULL_HANDLE;
        }
    }

    void VulkanCommand::createCommandPool() {
        const auto& queueFamilyIndices = m_device->getQueueFamilyIndices();

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(m_device->getLogicalDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void VulkanCommand::createCommandBuffers() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(m_device->getLogicalDevice(), &allocInfo, &m_commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void VulkanCommand::createSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(m_device->getLogicalDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(m_device->getLogicalDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(m_device->getLogicalDevice(), &fenceInfo, nullptr, &m_inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}
