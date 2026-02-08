#include "EnginePCH.h"
#include "RenderContextOpenGL.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Engine {

	RenderContextOpenGL::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		//USUL_CORE_ASSERT(windowHandle, "Window handle is null!")
	}

	void RenderContextOpenGL::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		//USUL_CORE_ASSERT(status, "Failed to initialize Glad!");

		//USUL_CORE_INFO("OpenGL Info:");
		//USUL_CORE_INFO("  Vendor: {0}", glGetString(GL_VENDOR));
		//USUL_CORE_INFO("  Renderer: {0}", glGetString(GL_RENDERER));
		//USUL_CORE_INFO("  Version: {0}", glGetString(GL_VERSION));
	}

	void RenderContextOpenGL::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}

	void RenderContextOpenGL::Cleanup()
	{
	}
}
