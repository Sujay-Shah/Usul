#version 450

// ------------------------------------------------------------------
// Deferred Lighting Pass — Fragment Shader
// Cook-Torrance PBR with Directional, Point, and Spot lights.
// Shadow map support for the primary directional light.
// ------------------------------------------------------------------

layout(location = 0) in vec2 v_TexCoord;

// G-Buffer inputs
layout(set = 0, binding = 0) uniform sampler2D u_GPosition;
layout(set = 0, binding = 1) uniform sampler2D u_GNormal;
layout(set = 0, binding = 2) uniform sampler2D u_GAlbedo;   // RGB = albedo, A = AO
layout(set = 0, binding = 3) uniform sampler2D u_GPBR;      // R = metallic, G = roughness

// Shadow map (optional — may be a 1x1 white texture when shadows off)
layout(set = 0, binding = 4) uniform sampler2DShadow u_ShadowMap;

// ---- Light data ----
struct LightData
{
    vec4  ColorIntensity;  // RGB = color, W = intensity
    vec4  Position;        // W = type (0=dir, 1=point, 2=spot)
    vec4  Direction;       // W = inner cutoff (spot)
    vec4  Params;          // X = outer cutoff, Y = radius, Z = castShadow, W = unused
};

layout(set = 1, binding = 0) uniform LightUBO
{
    LightData Lights[64];
    int       LightCount;
    vec3      CameraPos;
    mat4      LightSpaceMatrix; // for primary dir-light shadow
} u_Lights;

layout(location = 0) out vec4 o_Color;

const float PI = 3.14159265359;

// ---- PBR helpers ----
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdH = max(dot(N, H), 0.0);
    float d = (NdH * NdH * (a2 - 1.0) + 1.0);
    return a2 / (PI * d * d);
}

float GeometrySchlickGGX(float NdV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdV / (NdV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdV = max(dot(N, V), 0.0);
    float NdL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdV, roughness) * GeometrySchlickGGX(NdL, roughness);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ---- Shadow sampling ----
float ShadowFactor(vec3 worldPos)
{
    vec4 shadowCoord = u_Lights.LightSpaceMatrix * vec4(worldPos, 1.0);
    shadowCoord.xyz /= shadowCoord.w;
    // Map NDC [-1,1] → [0,1]
    shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;
    // PCF 3x3
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
    for (int x = -1; x <= 1; ++x)
    for (int y = -1; y <= 1; ++y)
        shadow += texture(u_ShadowMap, vec3(shadowCoord.xy + vec2(x, y) * texelSize, shadowCoord.z - 0.005));
    return shadow / 9.0;
}

// ---- Per-light radiance ----
vec3 EvalLight(LightData light, vec3 worldPos, vec3 N, vec3 V,
               vec3 albedo, float metallic, float roughness, vec3 F0)
{
    int  type = int(round(light.Position.w));
    vec3 L;
    float attenuation = 1.0;

    if (type == 0) // Directional
    {
        L = normalize(-light.Direction.xyz);
    }
    else if (type == 1) // Point
    {
        vec3 delta = light.Position.xyz - worldPos;
        float dist = length(delta);
        L = delta / dist;
        float radius = light.Params.y;
        attenuation = clamp(1.0 - (dist / radius), 0.0, 1.0);
        attenuation *= attenuation;
    }
    else // Spot
    {
        vec3 delta = light.Position.xyz - worldPos;
        float dist = length(delta);
        L = delta / dist;
        float theta     = dot(L, normalize(-light.Direction.xyz));
        float inner     = light.Direction.w;
        float outer     = light.Params.x;
        float epsilon   = inner - outer;
        attenuation     = clamp((theta - outer) / epsilon, 0.0, 1.0);
        float radius    = light.Params.y;
        float distAtten = clamp(1.0 - (dist / radius), 0.0, 1.0);
        attenuation    *= distAtten * distAtten;
    }

    vec3 H       = normalize(V + L);
    float NdL    = max(dot(N, L), 0.0);
    vec3  radiance = light.ColorIntensity.rgb * light.ColorIntensity.w * attenuation;

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    vec3 numerator   = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdL + 0.0001;
    vec3 specular    = numerator / denominator;

    return (kD * albedo / PI + specular) * radiance * NdL;
}

void main()
{
    // Sample G-Buffer
    vec3  worldPos  = texture(u_GPosition, v_TexCoord).rgb;
    vec3  N         = normalize(texture(u_GNormal,  v_TexCoord).rgb * 2.0 - 1.0);
    vec4  albedoAO  = texture(u_GAlbedo,   v_TexCoord);
    vec2  pbr       = texture(u_GPBR,      v_TexCoord).rg;

    vec3  albedo    = albedoAO.rgb;
    float ao        = albedoAO.a;
    float metallic  = pbr.r;
    float roughness = pbr.g;

    vec3 V  = normalize(u_Lights.CameraPos - worldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);

    for (int i = 0; i < u_Lights.LightCount; ++i)
    {
        vec3 contrib = EvalLight(u_Lights.Lights[i], worldPos, N, V, albedo, metallic, roughness, F0);

        // Apply shadow only for the first directional light that casts shadows
        if (i == 0 && u_Lights.Lights[i].Params.z > 0.5 && int(round(u_Lights.Lights[i].Position.w)) == 0)
            contrib *= ShadowFactor(worldPos);

        Lo += contrib;
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color   = ambient + Lo;

    // Simple Reinhard tone-map + gamma
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    o_Color = vec4(color, 1.0);
}
