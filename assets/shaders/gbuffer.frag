#version 450

// ------------------------------------------------------------------
// Deferred Geometry Pass — Fragment Shader
// Writes to four G-Buffer render targets:
//   RT0  RGBA16F  World-space position + depth
//   RT1  RGBA16F  World-space normal (encoded)
//   RT2  RGBA8    Albedo (RGB) + AO (A)
//   RT3  RGBA8    Metallic (R) + Roughness (G)
// ------------------------------------------------------------------

layout(location = 0) in vec3 v_WorldPos;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;
layout(location = 3) in mat3 v_TBN;
layout(location = 4) flat in int v_EntityID;

// Descriptor set 0 — material textures
layout(set = 0, binding = 0) uniform sampler2D u_AlbedoMap;
layout(set = 0, binding = 1) uniform sampler2D u_NormalMap;
layout(set = 0, binding = 2) uniform sampler2D u_MetallicRoughnessMap;
layout(set = 0, binding = 3) uniform sampler2D u_AOMap;

// Material scalar fallbacks (used when no texture is bound)
layout(set = 0, binding = 4) uniform MaterialUBO
{
    vec4  AlbedoColor;
    float Metallic;
    float Roughness;
    float AO;
    float HasAlbedoMap;
    float HasNormalMap;
    float HasMetallicRoughnessMap;
    float HasAOMap;
    float _pad;
} u_Material;

layout(location = 0) out vec4 o_Position; // RT0
layout(location = 1) out vec4 o_Normal;   // RT1
layout(location = 2) out vec4 o_Albedo;   // RT2
layout(location = 3) out vec4 o_PBR;      // RT3
layout(location = 4) out int o_EntityID;  // RT4

void main()
{
    // --- Albedo ---
    vec4 albedo = (u_Material.HasAlbedoMap > 0.5)
        ? texture(u_AlbedoMap, v_TexCoord)
        : u_Material.AlbedoColor;

    // Alpha cutout
    if (albedo.a < 0.01) discard;

    // --- Normal ---
    vec3 N;
    if (u_Material.HasNormalMap > 0.5)
    {
        vec3 tangentNormal = texture(u_NormalMap, v_TexCoord).rgb * 2.0 - 1.0;
        N = normalize(v_TBN * tangentNormal);
    }
    else
    {
        N = normalize(v_Normal);
    }

    // --- Metallic / Roughness ---
    float metallic, roughness;
    if (u_Material.HasMetallicRoughnessMap > 0.5)
    {
        vec4 mr = texture(u_MetallicRoughnessMap, v_TexCoord);
        metallic  = mr.b; // glTF convention: B = metallic, G = roughness
        roughness = mr.g;
    }
    else
    {
        metallic  = u_Material.Metallic;
        roughness = u_Material.Roughness;
    }

    // --- AO ---
    float ao = (u_Material.HasAOMap > 0.5)
        ? texture(u_AOMap, v_TexCoord).r
        : u_Material.AO;

    // --- Write G-Buffer ---
    o_Position = vec4(v_WorldPos, 1.0);
    o_Normal   = vec4(N * 0.5 + 0.5, 1.0); // pack [-1,1] → [0,1]
    o_Albedo   = vec4(albedo.rgb, ao);
    o_PBR      = vec4(metallic, roughness, 0.0, 1.0);
    o_EntityID = v_EntityID;
}
