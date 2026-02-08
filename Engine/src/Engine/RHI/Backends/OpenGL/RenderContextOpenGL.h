#pragma once

#include "Engine/RHI/RenderContext.h"

struct GLFWwindow;

namespace Engine {

	class RenderContextOpenGL : public RenderContext
	{
	public:
		RenderContextOpenGL(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void SwapBuffers() override;
		virtual void Cleanup() override;
	private:
		GLFWwindow* m_WindowHandle;
	};

}
