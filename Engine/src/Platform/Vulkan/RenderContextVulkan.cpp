//
// Created by snsha on 2024-02-01.
//

#include "RenderContextVulkan.h"
#include <set>

namespace Engine
{
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    RenderContextVulkan::RenderContextVulkan(GLFWwindow *windowHandle)
    : m_windowHandle(windowHandle)
    {
        if (m_windowHandle == NULL)
        {
            ENGINE_ERROR("Window handle is NULL!");
        }
    }

    void RenderContextVulkan::Init()
    {
        glfwMakeContextCurrent(m_windowHandle);

        if (m_enableValidationLayers && !checkValidationLayerSupport())
        {
            ENGINE_WARN("validation layers requested, but not available!");
        }

        //print available extensions
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        ENGINE_INFO("available extensions:");

        for (const auto& extension : extensions) {
            ENGINE_INFO("\t{0} version : {1}",extension.extensionName, extension.specVersion);
        }

        //TODO:: pass in creation info in constructor to have a uniform way of constructing render context for diff api
        //create vk instance
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Samples";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Usul";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        //validation layer config
        /*const VkBool32 setting_validate_core = VK_TRUE;
        const VkBool32 setting_validate_sync = VK_TRUE;
        const VkBool32 setting_thread_safety = VK_TRUE;
        const char* setting_debug_action[] = {"VK_DBG_LAYER_ACTION_LOG_MSG"};
        const char* setting_report_flags[] = {"info", "warn", "perf", "error", "debug"};
        const VkBool32 setting_enable_message_limit = VK_TRUE;
        const int32_t setting_duplicate_message_limit = 3;

        const VkLayerSettingEXT settings[] = {
                {layer_name, "validate_core", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_validate_core},
                {layer_name, "validate_sync", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_validate_sync},
                {layer_name, "thread_safety", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_thread_safety},
                {layer_name, "debug_action", VK_LAYER_SETTING_TYPE_STRING_EXT, 1, setting_debug_action},
                {layer_name, "report_flags", VK_LAYER_SETTING_TYPE_STRING_EXT, static_cast<uint32_t>(std::size(setting_report_flags)), setting_report_flags}
                {layer_name, "enable_message_limit", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &setting_enable_message_limit},
                {layer_name, "duplicate_message_limit", VK_LAYER_SETTING_TYPE_INT32_EXT, 1, &setting_duplicate_message_limit}};

        const VkLayerSettingsCreateInfoEXT layer_settings_create_info = {
                VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, nullptr,
                static_cast<uint32_t>(std::size(settings)), settings};

        const VkApplicationInfo app_info = initAppInfo();

        const char* layers[] = {layer_name};

        *//*
         * typedef struct VkInstanceCreateInfo {
            VkStructureType             sType;
            const void*                 pNext;
            VkInstanceCreateFlags       flags;
            const VkApplicationInfo*    pApplicationInfo;
            uint32_t                    enabledLayerCount;
            const char* const*          ppEnabledLayerNames;
            uint32_t                    enabledExtensionCount;
            const char* const*          ppEnabledExtensionNames;
        } VkInstanceCreateInfo;
         *//*
        const VkInstanceCreateInfo inst_create_info = {
                VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, &layer_settings_create_info, 0,
                &app_info,
                static_cast<uint32_t>(std::size(layers)), layers,
                0, nullptr
        };

        VkInstance instance = VK_NULL_HANDLE;
        VkResult result = vkCreateInstance(&inst_create_info, nullptr, &instance);*/

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

        //include validation layers
        if (m_enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
            createInfo.ppEnabledLayerNames = m_validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }
        //query extensions
        std::vector<const char*> requiredExtensions = getRequiredExtensions();
#if __APPLE__
        requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        createInfo.enabledExtensionCount = (uint32_t) requiredExtensions.size();
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        m_instance = VK_NULL_HANDLE;
        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
        if ( result!= VK_SUCCESS) {
            ENGINE_ERROR("failed to create instance!");
        }

       /* ENGINE_INFO("Vulkan Info:");
        ENGINE_INFO("    Vendor:   {0}", glGetString(GL_VENDOR));
        ENGINE_INFO("    Renderer: {0}", glGetString(GL_RENDERER));
        ENGINE_INFO("    Version:  {0}", glGetString(GL_VERSION));*/


    }

    void RenderContextVulkan::SwapBuffers()
    {


    }

    void RenderContextVulkan::Cleanup()
    {
        vkDestroyDevice(m_logicalDevice, nullptr);

        if (m_enableValidationLayers)
        {
            destroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
        }
        vkDestroyInstance(m_instance, nullptr);
    }

    bool RenderContextVulkan::checkValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        ENGINE_CORE_INFO("Available validation layers : ");
        for (const auto& layerProperties : availableLayers)
        {
            ENGINE_CORE_INFO("{0} version : {1}",layerProperties.layerName,layerProperties.implementationVersion);
        }
        for (const char* layerName : m_validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }

    std::vector<const char *> RenderContextVulkan::getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (m_enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        ENGINE_INFO("Extensions to be requested");
        for(auto ext : extensions)
        {
            ENGINE_INFO("{0}",ext);
        }

        return extensions;
    }

    void RenderContextVulkan::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void RenderContextVulkan::setupDebugMessenger()
    {
        if (!m_enableValidationLayers)
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
        {
            ENGINE_CORE_WARN("failed to setup debug messenger!");
        }

    }

    void
    RenderContextVulkan::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                       const VkAllocationCallbacks *pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }

    void RenderContextVulkan::pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            ENGINE_CORE_ERROR("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                m_physicalDevice = device;
                break;
            }
        }

        if (m_physicalDevice == VK_NULL_HANDLE) {
            ENGINE_CORE_ERROR("failed to find a suitable GPU!");
        }
    }

    bool RenderContextVulkan::isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        return indices.graphicsFamily.has_value();
    }

    QueueFamilyIndices RenderContextVulkan::findQueueFamilies(VkPhysicalDevice device) {
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

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    void RenderContextVulkan::createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

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

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());;
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        //device specific layer desc
        /*Previous implementations of Vulkan made a distinction between instance and device specific validation layers, but this is no longer the case.
         * That means that the enabledLayerCount and ppEnabledLayerNames fields of VkDeviceCreateInfo are ignored by up-to-date implementations.
         * However, it is still a good idea to set them anyway to be compatible with older implementations:*/
        if (m_enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
            createInfo.ppEnabledLayerNames = m_validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(m_logicalDevice, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_logicalDevice, indices.presentFamily.value(), 0, &m_presentQueue);
    }

    void RenderContextVulkan::createSurface()
    {
        if (glfwCreateWindowSurface(m_instance, m_windowHandle, nullptr, &m_surface) != VK_SUCCESS) {
            ENGINE_CORE_ERROR("failed to create window surface!");
        }
    }

    bool RenderContextVulkan::checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    SwapChainSupportDetails RenderContextVulkan::querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;

        return details;

    }
} // Engine