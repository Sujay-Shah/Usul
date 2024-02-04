//
// Created by snsha on 2024-02-01.
//

#include "RenderContextVulkan.h"

namespace Engine
{
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
        //ENGINE_ASSERT(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Glad failed to initialize!");

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

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        createInfo.enabledLayerCount = 0;

        if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
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
        vkDestroyInstance(m_instance, nullptr);
    }
} // Engine