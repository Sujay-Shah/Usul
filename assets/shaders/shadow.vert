#version 450

// ------------------------------------------------------------------
// Shadow Pass — Vertex Shader
// Depth-only pass for shadow map generation.
// ------------------------------------------------------------------

layout(location = 0) in vec3 a_Position;

layout(push_constant) uniform PushConstants
{
    mat4 u_LightSpaceMatrix;
    mat4 u_Model;
} pc;

void main()
{
    gl_Position = pc.u_LightSpaceMatrix * pc.u_Model * vec4(a_Position, 1.0);
}
