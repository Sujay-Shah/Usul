#pragma once
// ================================================================
//  SceneRenderer.h
//  Deferred rendering pipeline for the Usul engine.
//
//  Render order (each frame):
//   1. [Optional] Shadow Pass  — depth-only, directional light
//   2. Geometry   Pass         — fill G-Buffer (4 RTs)
//   3. Lighting   Pass         — full-screen quad, PBR evaluation
//
//  The final color result is available via GetColorOutput() and
//  can be displayed directly in the editor viewport (ImGui::Image)
//  or used as an input to post-processing.
// ================================================================

#include "Engine/Core/EngineDefines.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Components.h"
#include "Engine/Renderer/Camera/EditorCamera.h"
#include "RHI/rhi.hpp"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Engine
{

// ----------------------------------------------------------------
// Per-light GPU data layout (mirrors lighting.frag LightData)
// ----------------------------------------------------------------
struct GPULightData
{
    glm::vec4 ColorIntensity;  // RGB = color, W = intensity
    glm::vec4 Position;        // W = type  (0=dir,1=point,2=spot)
    glm::vec4 Direction;       // W = inner cutoff
    glm::vec4 Params;          // X=outerCutoff Y=radius Z=castShadow W=pad
};

// Light UBO — mirrors lighting.frag LightUBO
struct GPULightUBO
{
    GPULightData Lights[64];
    int32_t      LightCount;
    float        _pad0[3];
    glm::vec3    CameraPos;
    float        _pad1;
    glm::mat4    LightSpaceMatrix;
};

// Material UBO — mirrors gbuffer.frag MaterialUBO
struct GPUMaterialUBO
{
    glm::vec4 AlbedoColor;
    float     Metallic;
    float     Roughness;
    float     AO;
    float     HasAlbedoMap;
    float     HasNormalMap;
    float     HasMetallicRoughnessMap;
    float     HasAOMap;
    float     _pad;
};

// Push constants for geometry pass (64 bytes — fits in Vulkan minimum)
struct GeometryPushConstants
{
    glm::mat4 Model;
    glm::mat4 ViewProj;
};

// Push constants for shadow pass
struct ShadowPushConstants
{
    glm::mat4 LightSpaceMatrix;
    glm::mat4 Model;
};

// ----------------------------------------------------------------
//  Render settings — toggled from the Editor ImGui panel
// ----------------------------------------------------------------
struct RenderSettings
{
    bool EnableShadows    = false;
    bool EnableWireframe  = false;
    // Future: bloom, SSAO, etc.
};

// ----------------------------------------------------------------
//  SceneRenderer
// ----------------------------------------------------------------
class SceneRenderer
{
public:
    SceneRenderer() = default;
    ~SceneRenderer() = default;

    // Call once after RHI is up. Width/height are the initial viewport dimensions.
    void Init(uint32_t width, uint32_t height);

    // Resize G-Buffer and output textures. Triggers GPU wait.
    void Resize(uint32_t width, uint32_t height);

    // Submit one editor frame. Returns immediately (fence-waited inside).
    void RenderScene(const Ref<Scene>& scene, const EditorCamera& camera,
                     const RenderSettings& settings);

    // The final lit colour texture — display in the viewport.
    rhi::Texture GetColorOutput() const { return m_LightPassColor; }

    // Destroy all GPU resources (call before RHI shutdown).
    void Shutdown();

private:
    // ----- Resource creation helpers -----
    void CreateGBuffer(uint32_t w, uint32_t h);
    void DestroyGBuffer();
    void CreateLightPassTargets(uint32_t w, uint32_t h);
    void DestroyLightPassTargets();
    void CreateShadowMap(uint32_t res = 2048);
    void DestroyShadowMap();
    void CreatePipelines();
    void DestroyPipelines();
    void CreateDescriptorLayouts();
    void CreateSamplers();
    void DestroySamplers();

    // ----- Per-frame render steps -----
    void ShadowPass   (const rhi::CmdBuf& cmd, const Ref<Scene>& scene,
                       const glm::mat4& lightSpaceMatrix);
    void GeometryPass (const rhi::CmdBuf& cmd, const Ref<Scene>& scene,
                       const glm::mat4& view, const glm::mat4& proj);
    void LightingPass (const rhi::CmdBuf& cmd, const Ref<Scene>& scene,
                       const glm::vec3& cameraPos, const glm::mat4& lightSpaceMatrix,
                       const RenderSettings& settings);

    // Upload light data for the current scene into m_LightUBOs
    void UploadLightUBO(const Ref<Scene>& scene, const glm::vec3& cameraPos,
                        const glm::mat4& lightSpaceMatrix, uint32_t frameIndex);

    // Upload material UBO + write descriptors for one mesh entity
    void BindMaterial(const rhi::CmdBuf& cmd, const MaterialComponent& mat,
                      uint32_t frameIndex, uint32_t materialIdx);

    // Lazy-load GPU mesh data from a MeshComponent
    void EnsureMeshUploaded(MeshComponent& mc, const std::string& modelPath);

    // ----- G-Buffer targets -----
    rhi::Texture m_GPosition;   // RGBA16F  world-space position
    rhi::Texture m_GNormal;     // RGBA16F  world-space normal
    rhi::Texture m_GAlbedo;     // RGBA8    albedo + AO
    rhi::Texture m_GPBR;        // RGBA8    metallic + roughness
    rhi::Texture m_GDepth;      // D32_Float shared depth

    // ----- Lighting pass targets -----
    rhi::Texture m_LightPassColor; // RGBA16F final lit colour
    rhi::Texture m_LightPassDepth; // reuses m_GDepth ref (not owned again)

    // ----- Shadow map -----
    rhi::Texture m_ShadowMap;   // D32_Float
    uint32_t     m_ShadowRes = 2048;

    // ----- Pipelines -----
    rhi::Pipeline m_GeometryPipeline;
    rhi::Pipeline m_LightingPipeline;
    rhi::Pipeline m_ShadowPipeline;

    rhi::Shader   m_GBufferVS, m_GBufferFS;
    rhi::Shader   m_LightingVS, m_LightingFS;
    rhi::Shader   m_ShadowVS;

    // ----- Descriptor layouts -----
    rhi::DescriptorLayout m_MaterialLayout;   // set 0: material textures + UBO
    rhi::DescriptorLayout m_LightingLayout0;  // set 0: G-Buffer + shadow map + Light UBO

    static constexpr uint32_t k_MaxFrames = 3;

    rhi::DescriptorSet m_LightingSet;
    rhi::DescriptorSet m_MaterialSets[k_MaxFrames][64];
    rhi::Sampler m_LinearSampler;
    rhi::Sampler m_ShadowSampler; // comparison sampler for PCF

    // ----- Per-frame uniform buffers (triple-buffered) -----
    rhi::Buffer m_LightUBOs[k_MaxFrames];

    // Per-mesh material UBOs (one per entity per frame, simple slab)
    // We allocate a pool and bump-allocate each frame.
    rhi::Buffer m_MaterialUBOs[k_MaxFrames];
    uint32_t    m_MaterialUBOCount = 64; // max entities per frame

    // ----- Sync -----
    rhi::CmdBuf m_Cmd;
    rhi::Fence  m_Fence;

    // ----- Fallback 1x1 white texture -----
    rhi::Texture m_WhiteTex;
    // 1x1 depth texture for "no shadow" path
    rhi::Texture m_WhiteDepthTex;

    // ----- Dimensions -----
    uint32_t m_Width  = 1280;
    uint32_t m_Height = 720;

    bool m_Initialised = false;
};

} // namespace Engine
