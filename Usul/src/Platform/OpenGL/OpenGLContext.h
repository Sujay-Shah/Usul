#pragma once

#include "Renderer/GraphicsContext.h"

struct GLFWwindow;
namespace Usul
{
	class OpenGLContext : public GraphicsContext
	{
	public:
		OpenGLContext(GLFWwindow* windowHandle);


		void Init() override;

		void SwapBuffers() override;

	private:
		GLFWwindow* m_WindowHandle;

	};
}

