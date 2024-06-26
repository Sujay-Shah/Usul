#include "WindowGLFW.h"
#include "Event/KeyboardEvent.h"
#include "Event/WindowEvent.h"
#include "Event/MouseEvent.h"
#include "Platform/OpenGL/RenderContextOpenGL.h"
#include "Platform/Vulkan/RenderContextVulkan.h"
#include "Engine/Core/EngineDefines.h"

namespace Engine
{
    static void SetGLFWErrorCallback(int error, const char* message)
    {
        ENGINE_ERROR("GLFW Error ({0}): {1}", error, message);
    }

    Window* Window::Create(const WindowProps& props)
    {
        return new WindowGLFW(props);
    }

    WindowGLFW::WindowGLFW(const WindowProps& props)
    {
        Init(props);
    }

    WindowGLFW::~WindowGLFW()
    {
        Destroy();
    }

    void WindowGLFW::Init(const WindowProps& props)
    {
        ENGINE_ASSERT(glfwInit(), "GLFW failed to initialize!");
        ENGINE_INFO("GLFW initialized.");

        #ifdef __APPLE__
                    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
                    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
                    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        #endif

#if API_VULKAN
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif

        m_windowData = WindowProps(props);

        m_window = glfwCreateWindow(
        m_windowData.width, 
        m_windowData.height, 
        m_windowData.title.c_str(), NULL, NULL);
        SetVsync(true);

        ENGINE_ASSERT(m_window, "Window failed to be created!");
        ENGINE_INFO("Window created: {0}x{1}, {2}",
            m_windowData.width,
            m_windowData.height,
            m_windowData.title);
#if API_OPENGL
        m_renderContext = new RenderContextOpenGL(m_window);
#elif API_VULKAN
        m_renderContext = new RenderContextVulkan(m_window);
#endif
        m_renderContext->Init();

        glfwSetWindowUserPointer(m_window, &m_windowData);

        // GLFW callbacks
        glfwSetErrorCallback(SetGLFWErrorCallback);
        SetKeyCallback();
        SetCharCallback();
        SetWindowCloseCallback();
        SetWindowResizeCallback();
        SetMouseButtonCallback();
        SetMouseMoveCallback();
        SetMouseScrollCallback();
        
    }

    void WindowGLFW::Update()
    {
        glfwPollEvents();
        m_renderContext->SwapBuffers();
    }

    void WindowGLFW::Destroy()
    {
        m_renderContext->Cleanup();

        ENGINE_ERROR("Destroying window.");
        glfwDestroyWindow(m_window);
        ENGINE_ERROR("Terminating GLFW");
        glfwTerminate();
    }

    void WindowGLFW::SetKeyCallback()
    {
        glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            WindowProps& data = *(WindowProps*)glfwGetWindowUserPointer(window);
            
            switch(action)
            {
                case GLFW_PRESS:
                {
                    KeyPressedEvent e(key);
                    data.EventCallback(e);
                    break;
                }
                case GLFW_REPEAT:
                {
                    KeyPressedEvent e(key);
                    data.EventCallback(e);
                    break;
                }
                case GLFW_RELEASE:
                {
                    KeyReleasedEvent e(key);
                    data.EventCallback(e);
                    break;
                }
            }
        });
    }

    void WindowGLFW::SetCharCallback()
    {
        glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int key)
        {
            WindowProps& data = *(WindowProps*)glfwGetWindowUserPointer(window);

            KeyTypedEvent e(key);
            data.EventCallback(e);
        });
    }

    void WindowGLFW::SetWindowCloseCallback()
    {
        glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window)
        {
            WindowProps& data = *(WindowProps*)glfwGetWindowUserPointer(window);
            WindowCloseEvent e;

            data.EventCallback(e);
        });
    };

    void WindowGLFW::SetWindowResizeCallback()
    {
        #ifdef __APPLE__
        glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height)
        {
            WindowProps& data = *(WindowProps*)glfwGetWindowUserPointer(window);
            data.width = width;
            data.height = height;

            WindowResizeEvent e(width, height);

            data.EventCallback(e);
        });
        #else
        glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height)
        {
            WindowProps& data = *(WindowProps*)glfwGetWindowUserPointer(window);
            data.width = width;
            data.height = height;

            WindowResizeEvent e(width, height);

            data.EventCallback(e);
        });
        #endif
    };

    void WindowGLFW::SetMouseButtonCallback()
    {
        glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods)
        {
            WindowProps& data = *(WindowProps*)glfwGetWindowUserPointer(window);

            switch(action)
            {
                case GLFW_PRESS:
                {
                    MouseButtonPressedEvent e(button);
                    data.EventCallback(e);
                    break;
                }
                case GLFW_RELEASE:
                {
                    MouseButtonReleasedEvent e(button);
                    data.EventCallback(e);
                    break;
                }
            }
        });
    };

    void WindowGLFW::SetMouseMoveCallback()
    {
        glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xPos, double yPos)
        {
            WindowProps& data = *(WindowProps*)glfwGetWindowUserPointer(window);
            MouseMovedEvent e(xPos, yPos);
            data.EventCallback(e);
        });
    };

    void WindowGLFW::SetMouseScrollCallback()
    {
        glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xOffset, double yOffset)
        {
            WindowProps& data = *(WindowProps*)glfwGetWindowUserPointer(window);
            MouseScrolledEvent e(xOffset, yOffset);
            data.EventCallback(e);
        });
    };

    void WindowGLFW::SetVsync(bool enabled)
    {
        if(enabled)
        {
            glfwSwapInterval(1);
        }
        else
        {
            //glfwSwapInterval(0);
        }

        m_windowData.vsync = enabled;
    }

    void* WindowGLFW::GetNativeWindow() const
    {
        return m_window;
    }
}
