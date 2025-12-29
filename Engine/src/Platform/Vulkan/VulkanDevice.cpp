#include "EnginePCH.h"
#include "VulkanDevice.h"
#include <set>

namespace Engine {

    VulkanDevice::VulkanDevice(VkInstance instance, VkSurfaceKHR surface)
        : m_instance(instance), m_surface(surface) {
        pickPhysicalDevice();
        createLogicalDevice();
    }

    VulkanDevice::~VulkanDevice() {
        vkDestroyDevice(m_logicalDevice, nullptr);
    }

    void VulkanDevice::pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            ENGINE_CORE_ERROR("failed to find GPUs with Vulkan support!");
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            PhysicalDeviceInfo info;
            info.physicalDevice = device;
            vkGetPhysicalDeviceProperties(device, &info.properties);
            vkGetPhysicalDeviceFeatures(device, &info.features);
            vkGetPhysicalDeviceMemoryProperties(device, &info.memoryProperties);
            info.queueFamilyIndices = findQueueFamilies(device);
            
            if (isDeviceSuitable(info)) {
                m_physicals.push_back(info);
            }
        }

        if (m_physicals.empty()) {
            ENGINE_CORE_ERROR("failed to find a suitable GPU!");
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        // For now, just pick the first suitable device.
        m_physicalDevice = m_physicals[0].physicalDevice;
        m_physicalDeviceIndex = 0;

        ENGINE_CORE_INFO("Selected Physical Device: {0}", m_physicals[0].properties.deviceName);
    }

    bool VulkanDevice::isDeviceSuitable(const PhysicalDeviceInfo& info) {
        bool extensionsSupported = [this, &info]() {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(info.physicalDevice, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(info.physicalDevice, nullptr, &extensionCount, availableExtensions.data());

            std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

            for (const auto& extension : availableExtensions) {
                requiredExtensions.erase(extension.extensionName);
            }

            return requiredExtensions.empty();
        }();

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(info.physicalDevice, m_surface, &formatCount, nullptr);

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(info.physicalDevice, m_surface, &presentModeCount, nullptr);
            
            swapChainAdequate = formatCount > 0 && presentModeCount > 0;
        }

        return info.queueFamilyIndices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto &queueFamily: queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }
            
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    void VulkanDevice::createLogicalDevice() {
        const auto& indices = getQueueFamilyIndices();

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

        // Device specific layers are deprecated but good to have for older implementations
        // createInfo.enabledLayerCount = ...

        if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(m_logicalDevice, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_logicalDevice, indices.presentFamily.value(), 0, &m_presentQueue);
    }
}
