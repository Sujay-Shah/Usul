#version 450

// ------------------------------------------------------------------
// Deferred Geometry Pass — Vertex Shader
// Writes world-space position, normal, and UV to the G-Buffer.
// ------------------------------------------------------------------

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;

layout(push_constant) uniform PushConstants
{
    mat4 u_Model;
    mat4 u_ViewProj;
    int  u_EntityID;
} pc;

layout(location = 0) out vec3 v_WorldPos;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_TexCoord;
layout(location = 3) out mat3 v_TBN;
layout(location = 4) flat out int v_EntityID;

void main()
{
    vec4 worldPos = pc.u_Model * vec4(a_Position, 1.0);
    v_WorldPos    = worldPos.xyz;

    mat3 normalMat = transpose(inverse(mat3(pc.u_Model)));
    vec3 T = normalize(normalMat * a_Tangent);
    vec3 N = normalize(normalMat * a_Normal);
    T      = normalize(T - dot(T, N) * N); // re-orthogonalise
    vec3 B = cross(N, T);
    v_TBN  = mat3(T, B, N);

    v_Normal   = N;
    v_TexCoord = a_TexCoord;
    v_EntityID = pc.u_EntityID;

    gl_Position = pc.u_ViewProj * worldPos;
}
