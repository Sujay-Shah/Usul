#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>
#include <vector>

namespace Engine {

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    class VulkanDevice {
    public:
        struct PhysicalDeviceInfo {
            VkPhysicalDevice physicalDevice;
            VkPhysicalDeviceFeatures features;
            VkPhysicalDeviceProperties properties;
            VkPhysicalDeviceMemoryProperties memoryProperties;
            QueueFamilyIndices queueFamilyIndices;
        };

        VulkanDevice(VkInstance instance, VkSurfaceKHR surface);
        ~VulkanDevice();

        VkDevice getLogicalDevice() const { return m_logicalDevice; }
        VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
        const QueueFamilyIndices& getQueueFamilyIndices() const { return m_physicals[m_physicalDeviceIndex].queueFamilyIndices; }
        VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
        VkQueue getPresentQueue() const { return m_presentQueue; }

    private:
        void pickPhysicalDevice();
        bool isDeviceSuitable(const PhysicalDeviceInfo& info);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        void createLogicalDevice();

    private:
        VkInstance m_instance;
        VkSurfaceKHR m_surface;
        std::vector<PhysicalDeviceInfo> m_physicals;
        size_t m_physicalDeviceIndex = 0;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_logicalDevice;
        VkQueue m_graphicsQueue;
        VkQueue m_presentQueue;

        const std::vector<const char*> m_deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
#if __APPLE__
            ,"VK_KHR_portability_subset"
#endif
        };
    };
}
