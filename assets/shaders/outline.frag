#version 450

// ------------------------------------------------------------------
// Outline Pass — Fragment Shader
// Outputs a solid color.
// ------------------------------------------------------------------

layout(push_constant) uniform PushConstants
{
    mat4 u_Model;
    mat4 u_ViewProj;
    vec4 u_Color;
} pc;

layout(location = 0) out vec4 o_Color;

void main()
{
    o_Color = pc.u_Color;
}
