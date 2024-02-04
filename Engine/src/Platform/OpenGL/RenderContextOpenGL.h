#ifndef __OPENGLCONTEXT_H__
#define __OPENGLCONTEXT_H__

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Renderer/RenderContext.h"

struct GLFWwindow;

namespace Engine
{
    class RenderContextOpenGL : public RenderContext
    {
        public:
            RenderContextOpenGL(GLFWwindow* windowHandle);

            virtual void Init() override;
            virtual void SwapBuffers() override;

        void Cleanup() override;

    private:
            GLFWwindow* m_windowHandle = nullptr;
    };
}

#endif