#include "BufferOpenGL.h"
#include "Engine/Core/Logging.h"

#include <glad/glad.h>

namespace Engine
{
    VertexBufferOpenGL::VertexBufferOpenGL(float* vertices, uint32_t size)
    {
        m_size = size;
        glGenBuffers(1, &m_rendererID);
        glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
        glBufferData(GL_ARRAY_BUFFER,size, vertices, GL_STATIC_DRAW);

        ENGINE_INFO("Created OpenGL Vertex Buffer.");
    }

    VertexBufferOpenGL::~VertexBufferOpenGL()
    {
        glDeleteBuffers(1, &m_rendererID);
    }

    void VertexBufferOpenGL::Bind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
    }

    void VertexBufferOpenGL::UnBind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    VertexBufferOpenGL::VertexBufferOpenGL(Vertex *vertices, uint32_t size)
    {
        m_size = size;
        glGenBuffers(1, &m_rendererID);
        glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
        glBufferData(GL_ARRAY_BUFFER, size * sizeof (Vertex), vertices, GL_STATIC_DRAW);
        //ENGINE_CORE_INFO("glBufferData(GL_ARRAY_BUFFER, {0}, vertices, GL_STATIC_DRAW)",size * sizeof (Vertex));
    }

    IndexBufferOpenGL::IndexBufferOpenGL(uint32_t* indices, uint32_t count)
    :
    m_count(count)
    {
        glGenBuffers(1, &m_rendererID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
        //ENGINE_CORE_INFO("glBufferData(GL_ELEMENT_ARRAY_BUFFER, {0}, indices, GL_STATIC_DRAW)",count * sizeof(uint32_t));
        ENGINE_INFO("Created OpenGL Index Buffer.");
    }

    IndexBufferOpenGL::~IndexBufferOpenGL()
    {
        glDeleteBuffers(1, &m_rendererID);
    }

    void IndexBufferOpenGL::
    Bind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID);
    }

    void IndexBufferOpenGL::UnBind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}