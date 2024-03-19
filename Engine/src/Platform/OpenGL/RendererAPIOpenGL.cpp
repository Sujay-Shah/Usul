#include "RendererAPIOpenGL.h"

#include <glad/glad.h>

namespace Engine
{
    void RendererAPIOpenGL::Init()
    {
        //glEnable(GL_BLEND);
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
    }

    void RendererAPIOpenGL::SetClearColor(const glm::vec4& color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void RendererAPIOpenGL::Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void RendererAPIOpenGL::DrawIndexed(const Ref<VertexArray>& vertexArray)
    {
        if (vertexArray->GetIndexBuffer().get())
        {
            glDrawElements(GL_TRIANGLES, vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
        }
    }

    void RendererAPIOpenGL::SetViewPort(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        glViewport(x, y, width, height);
    }

	void RendererAPIOpenGL::DrawArrays(const Ref<VertexArray>& vertexArray)
	{
        for (int i = 0; i<vertexArray->GetVertexBuffers().size();++i)
        {
		    glDrawArrays(GL_TRIANGLES, 0, vertexArray->GetVertexBuffers()[i]->GetSize());
        }
	}

}
