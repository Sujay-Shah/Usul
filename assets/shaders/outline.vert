#version 450

// ------------------------------------------------------------------
// Outline Pass — Vertex Shader
// Scales the mesh up slightly to draw an outline.
// ------------------------------------------------------------------

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;

layout(push_constant) uniform PushConstants
{
    mat4 u_Model;
    mat4 u_ViewProj;
    vec4 u_Color;
} pc;

void main()
{
    // Extrude along normal slightly to create outline effect
    vec3 position = a_Position + a_Normal * 0.05; // 0.05 is the outline thickness
    vec4 worldPos = pc.u_Model * vec4(position, 1.0);
    gl_Position = pc.u_ViewProj * worldPos;
}
