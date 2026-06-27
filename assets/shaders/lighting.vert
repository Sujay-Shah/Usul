#version 450

// ------------------------------------------------------------------
// Deferred Lighting Pass — Vertex Shader
// Full-screen triangle (no vertex buffer needed).
// ------------------------------------------------------------------

layout(location = 0) out vec2 v_TexCoord;

void main()
{
    // Emit a full-screen triangle covering [-1,1] NDC using gl_VertexIndex
    vec2 uv  = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    v_TexCoord = vec2(uv.x, 1.0 - uv.y);
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
