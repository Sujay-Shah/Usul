#pragma once

#include "RHI/SwapChain.h"
#include <vulkan/vulkan.hpp>
#include <vector>

struct GLFWwindow;

namespace Engine {

    class VulkanDevice;

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class VulkanSwapChain : public SwapChain {
    public:
        VulkanSwapChain(const std::shared_ptr<VulkanDevice>& device, VkSurfaceKHR surface, GLFWwindow* window);
        ~VulkanSwapChain();

        void Create() override;
        void Cleanup() override;

        uint32_t GetWidth() const override;
        uint32_t GetHeight() const override;

        VkSwapchainKHR getSwapChain() const { return m_swapChain; }
        const std::vector<VkImage>& getImages() const { return m_swapChainImages; }
        VkFormat getImageFormat() const { return m_swapChainImageFormat; }
        const VkExtent2D& getExtent() const { return m_swapChainExtent; }
        const std::vector<VkImageView>& getImageViews() const { return m_swapChainImageViews; }
        VkSurfaceKHR getSurface() const {return m_surface; }
    private:
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        void createImageViews();

    private:
        std::shared_ptr<VulkanDevice> m_device;
        VkSurfaceKHR m_surface;
        GLFWwindow* m_windowHandle;

        VkSwapchainKHR m_swapChain;
        std::vector<VkImage> m_swapChainImages;
        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;
        std::vector<VkImageView> m_swapChainImageViews;
    };
}
