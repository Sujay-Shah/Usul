#include "EnginePCH.h"
#include "SceneRenderer.h"
#include "Engine/Core/AssetManager.h"
#include "Engine/Renderer/Model.h"
#include "Engine/Renderer/Vertex.h"
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <vector>

namespace Engine {

static std::vector<char> ReadSpv(const std::string& path)
{
    std::ifstream f(path, std::ios::ate | std::ios::binary);
    if (!f.is_open()) {
        ENGINE_CORE_ERROR("SceneRenderer: Cannot open shader: {0}", path);
        ENGINE_DEBUGBREAK();
    }
    size_t size = (size_t)f.tellg();
    std::vector<char> buf(size);
    f.seekg(0);
    f.read(buf.data(), size);
    return buf;
}

// ================================================================
//  Init / Shutdown
// ================================================================
void SceneRenderer::Init(uint32_t width, uint32_t height)
{
    if (m_Initialised) return;
    m_Width  = width;
    m_Height = height;

    CreateSamplers();
    CreateDescriptorLayouts();
    CreatePipelines();
    CreateGBuffer(width, height);
    CreateLightPassTargets(width, height);
    CreateShadowMap(m_ShadowRes);

    m_LightingSet = rhi::descriptor_set_create(m_LightingLayout0);

    // Per-frame UBOs and material descriptor sets
    for (uint32_t i = 0; i < k_MaxFrames; ++i)
    {
        m_LightUBOs[i] = rhi::buffer_create({
            .size   = sizeof(GPULightUBO),
            .usage  = rhi::BufferUsage::Uniform,
            .memory = rhi::MemoryType::CpuToGpu,
            .name   = "LightUBO"
        });

        for (uint32_t j = 0; j < 64; ++j)
        {
            m_MaterialSets[i][j] = rhi::descriptor_set_create(m_MaterialLayout);
            m_MaterialUBOs[i][j] = rhi::buffer_create({
                .size   = sizeof(GPUMaterialUBO),
                .usage  = rhi::BufferUsage::Uniform,
                .memory = rhi::MemoryType::CpuToGpu,
                .name   = "MaterialUBO"
            });
        }
    }

    m_EntityIDReadbackBuffer = rhi::buffer_create({
        .size   = sizeof(int),
        .usage  = rhi::BufferUsage::TransferDst,
        .memory = rhi::MemoryType::GpuToCpu,
        .name   = "EntityIDReadback"
    });

    // Fallback 1x1 white texture
    {
        uint32_t white = 0xFFFFFFFF;
        m_WhiteTex = rhi::texture_create({
            .width=1,.height=1,
            .format=rhi::Format::RGBA8_Unorm,
            .usage=rhi::TextureUsage::Sampled|rhi::TextureUsage::TransferDst,
            .name="White1x1"
        });
        auto ctx = rhi::UploadContext::create(4);
        ctx.begin();
        ctx.upload_texture(m_WhiteTex, &white, 4, 1, 1);
        ctx.submit_and_wait();
        ctx.destroy();
    }

    m_Cmd   = rhi::cmdbuf_create(0);
    m_Fence = rhi::fence_create(false);
    m_Initialised = true;
    ENGINE_CORE_INFO("SceneRenderer: Initialised ({0}x{1})", width, height);
}

void SceneRenderer::Shutdown()
{
    if (!m_Initialised) return;
    rhi::device_wait_idle();

    DestroyGBuffer();
    DestroyLightPassTargets();
    DestroyShadowMap();
    DestroyPipelines();
    DestroySamplers();

    for (uint32_t i = 0; i < k_MaxFrames; ++i)
    {
        rhi::buffer_destroy(m_LightUBOs[i]);
        for (uint32_t j = 0; j < 64; ++j)
        {
            rhi::buffer_destroy(m_MaterialUBOs[i][j]);
            rhi::descriptor_set_destroy(m_MaterialSets[i][j]);
        }
    }

    rhi::buffer_destroy(m_EntityIDReadbackBuffer);

    rhi::texture_destroy(m_WhiteTex);
    rhi::cmdbuf_destroy(m_Cmd);
    rhi::fence_destroy(m_Fence);

    rhi::descriptor_layout_destroy(m_MaterialLayout);
    rhi::descriptor_layout_destroy(m_LightingLayout0);
    rhi::descriptor_set_destroy(m_LightingSet);

    m_Initialised = false;
}

// ================================================================
//  Resize
// ================================================================
void SceneRenderer::Resize(uint32_t w, uint32_t h)
{
    if (w == m_Width && h == m_Height) return;
    rhi::device_wait_idle();
    m_Width = w; m_Height = h;

    DestroyGBuffer();
    DestroyLightPassTargets();
    CreateGBuffer(w, h);
    CreateLightPassTargets(w, h);
}

int SceneRenderer::GetEntityAtPixel(int x, int y)
{
    if (x < 0 || y < 0 || x >= m_Width || y >= m_Height)
        return -1;

    // We must wait for the GPU to finish rendering to m_GEntityID before copying from it
    rhi::device_wait_idle();

    // Create a temporary command buffer for the transfer
    rhi::CmdBuf cmd = rhi::cmdbuf_create(0);
    rhi::Fence fence = rhi::fence_create(false);

    rhi::cmdbuf_begin(cmd);

    // Transition texture to transfer source
    rhi::TextureBarrier barSrc = {
        .tex = m_GEntityID,
        .old_layout = rhi::TextureLayout::ColorTarget,
        .new_layout = rhi::TextureLayout::TransferSrc,
        .src_stage = rhi::PipelineStage::ColorOutput,
        .dst_stage = rhi::PipelineStage::Transfer,
        .src_access = rhi::Access::ColorWrite,
        .dst_access = rhi::Access::TransferRead,
        .base_mip = 0, .mip_count = 1, .base_layer = 0, .layer_count = 1
    };
    rhi::texture_barrier(cmd, &barSrc, 1);

    // Copy 1 pixel to readback buffer
    rhi::BufferTextureCopy copy = {
        .buf = m_EntityIDReadbackBuffer,
        .buf_offset = 0,
        .buf_row_len = 0,
        .buf_img_h = 0,
        .tex = m_GEntityID,
        .mip = 0,
        .base_layer = 0,
        .layer_count = 1,
        .x = (uint32_t)x,
        .y = (uint32_t)(m_Height - 1 - y),
        .z = 0,
        .w = 1,
        .h = 1,
        .d = 1
    };
    rhi::copy_texture_to_buffer(cmd, copy);

    // Transition texture back to color target (for next frame)
    rhi::TextureBarrier barDst = {
        .tex = m_GEntityID,
        .old_layout = rhi::TextureLayout::TransferSrc,
        .new_layout = rhi::TextureLayout::ColorTarget,
        .src_stage = rhi::PipelineStage::Transfer,
        .dst_stage = rhi::PipelineStage::ColorOutput,
        .src_access = rhi::Access::TransferRead,
        .dst_access = rhi::Access::ColorWrite,
        .base_mip = 0, .mip_count = 1, .base_layer = 0, .layer_count = 1
    };
    rhi::texture_barrier(cmd, &barDst, 1);

    rhi::cmdbuf_end(cmd);
    rhi::queue_submit(&cmd, 1, nullptr, 0, nullptr, 0, fence);
    rhi::fence_wait(fence);

    rhi::cmdbuf_destroy(cmd);
    rhi::fence_destroy(fence);

    auto mapped = rhi::buffer_map(m_EntityIDReadbackBuffer);
    int entityID = *(int*)mapped.ptr;
    rhi::buffer_unmap(m_EntityIDReadbackBuffer);

    return entityID;
}

// ================================================================
//  G-Buffer helpers
// ================================================================
void SceneRenderer::CreateGBuffer(uint32_t w, uint32_t h)
{
    auto mkTex = [&](rhi::Format fmt, rhi::TextureUsage usage, const char* name) {
        return rhi::texture_create({ .width=w,.height=h,.format=fmt,.usage=usage,.name=name });
    };
    using TU = rhi::TextureUsage;
    m_GPosition = mkTex(rhi::Format::RGBA16_Float, TU::ColorTarget|TU::Sampled, "GPosition");
    m_GNormal   = mkTex(rhi::Format::RGBA16_Float, TU::ColorTarget|TU::Sampled, "GNormal");
    m_GAlbedo   = mkTex(rhi::Format::RGBA8_Unorm,  TU::ColorTarget|TU::Sampled, "GAlbedo");
    m_GPBR      = mkTex(rhi::Format::RGBA8_Unorm,  TU::ColorTarget|TU::Sampled, "GPBR");
    m_GEntityID = mkTex(rhi::Format::R32_Uint,     TU::ColorTarget|TU::TransferSrc, "GEntityID");
    m_GDepth    = mkTex(rhi::Format::D32_Float,    TU::DepthTarget,             "GDepth");
}

void SceneRenderer::DestroyGBuffer()
{
    rhi::texture_destroy(m_GPosition);
    rhi::texture_destroy(m_GNormal);
    rhi::texture_destroy(m_GAlbedo);
    rhi::texture_destroy(m_GPBR);
    rhi::texture_destroy(m_GEntityID);
    rhi::texture_destroy(m_GDepth);
}

void SceneRenderer::CreateLightPassTargets(uint32_t w, uint32_t h)
{
    m_LightPassColor = rhi::texture_create({
        .width=w,.height=h,
        .format=rhi::Format::RGBA16_Float,
        .usage=rhi::TextureUsage::ColorTarget|rhi::TextureUsage::Sampled,
        .name="LightPassColor"
    });
}

void SceneRenderer::DestroyLightPassTargets()
{
    rhi::texture_destroy(m_LightPassColor);
}

void SceneRenderer::CreateShadowMap(uint32_t res)
{
    m_ShadowRes = res;
    m_ShadowMap = rhi::texture_create({
        .width=res,.height=res,
        .format=rhi::Format::D32_Float,
        .usage=rhi::TextureUsage::DepthTarget|rhi::TextureUsage::Sampled,
        .name="ShadowMap"
    });
}

void SceneRenderer::DestroyShadowMap()
{
    rhi::texture_destroy(m_ShadowMap);
}

// ================================================================
//  Samplers
// ================================================================
void SceneRenderer::CreateSamplers()
{
    m_LinearSampler = rhi::sampler_create({
        .min_filter=rhi::SamplerFilter::Linear,
        .mag_filter=rhi::SamplerFilter::Linear,
        .mip_mode=rhi::SamplerMipmap::Linear,
        .name="LinearRepeat"
    });
    m_ShadowSampler = rhi::sampler_create({
        .min_filter=rhi::SamplerFilter::Linear,
        .mag_filter=rhi::SamplerFilter::Linear,
        .address_u=rhi::SamplerAddress::ClampToEdge,
        .address_v=rhi::SamplerAddress::ClampToEdge,
        .compare=false,
        .compare_op=rhi::CompareOp::Always,
        .name="ShadowPCF"
    });
}

void SceneRenderer::DestroySamplers()
{
    rhi::sampler_destroy(m_LinearSampler);
    rhi::sampler_destroy(m_ShadowSampler);
}

// ================================================================
//  Descriptor layouts
// ================================================================
void SceneRenderer::CreateDescriptorLayouts()
{
    using DT = rhi::DescriptorType;
    using SS = rhi::ShaderStage;

    // set 0 for geometry pass: 4 combined-image-samplers + 1 UBO
    m_MaterialLayout = rhi::descriptor_layout_create({
        .bindings = {
            {.binding=0,.type=DT::CombinedImageSampler,.count=1,.stages=SS::Fragment},
            {.binding=1,.type=DT::CombinedImageSampler,.count=1,.stages=SS::Fragment},
            {.binding=2,.type=DT::CombinedImageSampler,.count=1,.stages=SS::Fragment},
            {.binding=3,.type=DT::CombinedImageSampler,.count=1,.stages=SS::Fragment},
            {.binding=4,.type=DT::UniformBuffer,        .count=1,.stages=SS::Fragment},
        },
        .count=5, .name="MaterialLayout"
    });

    // set 0 for lighting pass: 4 G-Buffer + shadow map + light UBO
    m_LightingLayout0 = rhi::descriptor_layout_create({
        .bindings = {
            {.binding=0,.type=DT::CombinedImageSampler,.count=1,.stages=SS::Fragment},
            {.binding=1,.type=DT::CombinedImageSampler,.count=1,.stages=SS::Fragment},
            {.binding=2,.type=DT::CombinedImageSampler,.count=1,.stages=SS::Fragment},
            {.binding=3,.type=DT::CombinedImageSampler,.count=1,.stages=SS::Fragment},
            {.binding=4,.type=DT::CombinedImageSampler,.count=1,.stages=SS::Fragment},
            {.binding=5,.type=DT::UniformBuffer,       .count=1,.stages=SS::Fragment},
        },
        .count=6, .name="LightingGBufLayout"
    });
}

// ================================================================
//  Pipelines
// ================================================================
void SceneRenderer::CreatePipelines()
{
    auto loadSpv = [](const std::string& p) { return ReadSpv(AssetManager::GetAssetPath(p).string()); };

    auto gvert = loadSpv("shaders/gbuffer.vert.spv");
    auto gfrag = loadSpv("shaders/gbuffer.frag.spv");
    auto lvert = loadSpv("shaders/lighting.vert.spv");
    auto lfrag = loadSpv("shaders/lighting.frag.spv");
    auto svert = loadSpv("shaders/shadow.vert.spv");

    m_GBufferVS  = rhi::shader_create({.bytecode=gvert.data(),.size=gvert.size(),.stage=rhi::ShaderStage::Vertex,  .name="GBufVS"});
    m_GBufferFS  = rhi::shader_create({.bytecode=gfrag.data(),.size=gfrag.size(),.stage=rhi::ShaderStage::Fragment,.name="GBufFS"});
    m_LightingVS = rhi::shader_create({.bytecode=lvert.data(),.size=lvert.size(),.stage=rhi::ShaderStage::Vertex,  .name="LightVS"});
    m_LightingFS = rhi::shader_create({.bytecode=lfrag.data(),.size=lfrag.size(),.stage=rhi::ShaderStage::Fragment,.name="LightFS"});
    m_ShadowVS   = rhi::shader_create({.bytecode=svert.data(),.size=svert.size(),.stage=rhi::ShaderStage::Vertex,  .name="ShadowVS"});

    auto overt = loadSpv("shaders/outline.vert.spv");
    auto ofrag = loadSpv("shaders/outline.frag.spv");
    m_OutlineVS  = rhi::shader_create({.bytecode=overt.data(),.size=overt.size(),.stage=rhi::ShaderStage::Vertex,  .name="OutlineVS"});
    m_OutlineFS  = rhi::shader_create({.bytecode=ofrag.data(),.size=ofrag.size(),.stage=rhi::ShaderStage::Fragment,.name="OutlineFS"});

    // Geometry pipeline — writes to 4 color targets + depth
    m_GeometryPipeline = rhi::pipeline_create({
        .vertex_shader   = m_GBufferVS,
        .fragment_shader = m_GBufferFS,
        .layout          = m_MaterialLayout,
        .attribs = {
            {.binding=0,.location=0,.format=rhi::Format::RGB32_Float,.offset=offsetof(Vertex,Position)},
            {.binding=0,.location=1,.format=rhi::Format::RGB32_Float,.offset=offsetof(Vertex,Normal)},
            {.binding=0,.location=2,.format=rhi::Format::RG32_Float, .offset=offsetof(Vertex,TexCoords)},
            {.binding=0,.location=3,.format=rhi::Format::RGB32_Float,.offset=offsetof(Vertex,Tangent)},
            {.binding=0,.location=4,.format=rhi::Format::RGB32_Float,.offset=offsetof(Vertex,Bitangent)},
        },
        .attrib_count  = 5,
        .bindings      = {{.binding=0,.stride=sizeof(Vertex),.input_rate=rhi::VertexInputRate::Vertex}},
        .binding_count = 1,
        .color_formats = {rhi::Format::RGBA16_Float,rhi::Format::RGBA16_Float,rhi::Format::RGBA8_Unorm,rhi::Format::RGBA8_Unorm,rhi::Format::R32_Uint},
        .color_count   = 5,
        .depth_format  = rhi::Format::D32_Float,
        .depth         = {.test_enable=true,.write_enable=true,.compare_op=rhi::CompareOp::Less},
        .push_constant_size = sizeof(GeometryPushConstants),
        .name = "GeometryPipeline"
    });

    m_GeometryWireframePipeline = rhi::pipeline_create({
        .vertex_shader   = m_GBufferVS,
        .fragment_shader = m_GBufferFS,
        .layout          = m_MaterialLayout,
        .attribs = {
            {.binding=0,.location=0,.format=rhi::Format::RGB32_Float,.offset=offsetof(Vertex,Position)},
            {.binding=0,.location=1,.format=rhi::Format::RGB32_Float,.offset=offsetof(Vertex,Normal)},
            {.binding=0,.location=2,.format=rhi::Format::RG32_Float, .offset=offsetof(Vertex,TexCoords)},
            {.binding=0,.location=3,.format=rhi::Format::RGB32_Float,.offset=offsetof(Vertex,Tangent)},
            {.binding=0,.location=4,.format=rhi::Format::RGB32_Float,.offset=offsetof(Vertex,Bitangent)},
        },
        .attrib_count  = 5,
        .bindings      = {{.binding=0,.stride=sizeof(Vertex),.input_rate=rhi::VertexInputRate::Vertex}},
        .binding_count = 1,
        .color_formats = {rhi::Format::RGBA16_Float,rhi::Format::RGBA16_Float,rhi::Format::RGBA8_Unorm,rhi::Format::RGBA8_Unorm,rhi::Format::R32_Uint},
        .color_count   = 5,
        .depth_format  = rhi::Format::D32_Float,
        .depth         = {.test_enable=true,.write_enable=true,.compare_op=rhi::CompareOp::Less},
        .raster        = {.fill_mode=rhi::FillMode::Wireframe},
        .push_constant_size = sizeof(GeometryPushConstants),
        .name = "GeometryWireframePipeline"
    });

    // Lighting pipeline — full-screen quad, no vertex input, one color target
    m_LightingPipeline = rhi::pipeline_create({
        .vertex_shader   = m_LightingVS,
        .fragment_shader = m_LightingFS,
        .layout          = m_LightingLayout0,
        .color_formats   = {rhi::Format::RGBA16_Float},
        .color_count     = 1,
        .depth           = {.test_enable=false,.write_enable=false},
        .name = "LightingPipeline"
    });

    // Shadow pipeline — depth-only, no fragment shader
    m_ShadowPipeline = rhi::pipeline_create({
        .vertex_shader = m_ShadowVS,
        .attribs       = {{.binding=0,.location=0,.format=rhi::Format::RGB32_Float,.offset=offsetof(Vertex,Position)}},
        .attrib_count  = 1,
        .bindings      = {{.binding=0,.stride=sizeof(Vertex),.input_rate=rhi::VertexInputRate::Vertex}},
        .binding_count = 1,
        .depth_format  = rhi::Format::D32_Float,
        .depth         = {.test_enable=true,.write_enable=true,.compare_op=rhi::CompareOp::Less},
        .raster        = {.cull_mode=rhi::CullMode::Front}, // peter-panning fix
        .push_constant_size = sizeof(ShadowPushConstants),
        .name = "ShadowPipeline"
    });

    struct OutlinePushConstants {
        glm::mat4 Model;
        glm::mat4 ViewProj;
        glm::vec4 Color;
    };

    m_OutlinePipeline = rhi::pipeline_create({
        .vertex_shader   = m_OutlineVS,
        .fragment_shader = m_OutlineFS,
        .layout          = rhi::DescriptorLayout{}, // no layout needed
        .attribs = {
            {.binding=0,.location=0,.format=rhi::Format::RGB32_Float,.offset=offsetof(Vertex,Position)},
            {.binding=0,.location=1,.format=rhi::Format::RGB32_Float,.offset=offsetof(Vertex,Normal)},
        },
        .attrib_count  = 2,
        .bindings      = {{.binding=0,.stride=sizeof(Vertex),.input_rate=rhi::VertexInputRate::Vertex}},
        .binding_count = 1,
        .color_formats = {rhi::Format::RGBA16_Float}, // renders directly to LightPassColor
        .color_count   = 1,
        .depth_format  = rhi::Format::D32_Float,
        // LessEqual so it renders over the existing depth, write_enable=false
        .depth         = {.test_enable=true,.write_enable=false,.compare_op=rhi::CompareOp::LessEqual},
        .raster        = {.cull_mode=rhi::CullMode::Front}, // Render back faces for outline
        .push_constant_size = sizeof(OutlinePushConstants),
        .name = "OutlinePipeline"
    });
}

void SceneRenderer::DestroyPipelines()
{
    rhi::pipeline_destroy(m_GeometryPipeline);
    rhi::pipeline_destroy(m_GeometryWireframePipeline);
    rhi::pipeline_destroy(m_LightingPipeline);
    rhi::pipeline_destroy(m_ShadowPipeline);
    rhi::pipeline_destroy(m_OutlinePipeline);
    rhi::shader_destroy(m_GBufferVS);  rhi::shader_destroy(m_GBufferFS);
    rhi::shader_destroy(m_LightingVS); rhi::shader_destroy(m_LightingFS);
    rhi::shader_destroy(m_ShadowVS);
    rhi::shader_destroy(m_OutlineVS);  rhi::shader_destroy(m_OutlineFS);
}

// ================================================================
//  Light Space Matrix helper (orthographic, sun-like directional)
// ================================================================
static glm::mat4 CalcLightSpaceMatrix(const glm::vec3& direction)
{
    glm::vec3 dir = glm::normalize(direction);
    glm::mat4 lightView = glm::lookAt(-dir * 50.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 lightProj = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, 0.1f, 200.0f);
    return lightProj * lightView;
}

// ================================================================
//  UploadLightUBO
// ================================================================
void SceneRenderer::UploadLightUBO(const Ref<Scene>& scene,
                                    const glm::vec3& cameraPos,
                                    const glm::mat4& lightSpaceMatrix,
                                    uint32_t /*frameIndex*/,
                                    const RenderSettings& settings)
{
    GPULightUBO ubo{};
    ubo.CameraPos        = cameraPos;
    ubo.LightSpaceMatrix = lightSpaceMatrix;

    auto view = scene->GetRegistry().view<TransformComponent, LightComponent>();
    for (auto entity : view)
    {
        if (ubo.LightCount >= 64) break;
        auto [tc, lc] = view.get<TransformComponent, LightComponent>(entity);
        GPULightData& ld = ubo.Lights[ubo.LightCount++];

        ld.ColorIntensity = { lc.Color, lc.Intensity };
        ld.Position       = { tc.Translation, (float)lc.Type };
        // Direction from rotation (simple: use -Y rotated by transform)
        glm::vec3 dir = glm::normalize(glm::vec3(
            glm::sin(tc.Rotation.y) * glm::cos(tc.Rotation.x),
           -glm::sin(tc.Rotation.x),
           -glm::cos(tc.Rotation.y) * glm::cos(tc.Rotation.x)));
        ld.Direction = { dir, lc.InnerCutoff };
        ld.Params    = { lc.OuterCutoff, lc.Radius, (settings.EnableShadows && lc.CastShadows) ? 1.0f : 0.0f, 0.0f };
    }

    auto mapped = rhi::buffer_map(m_LightUBOs[0]);
    memcpy(mapped.ptr, &ubo, sizeof(ubo));
    rhi::buffer_unmap(m_LightUBOs[0]);
}

void SceneRenderer::BindMaterial(const rhi::CmdBuf& cmd, const MaterialComponent& mat,
                                  uint32_t frameIndex, uint32_t materialIdx)
{
    if (materialIdx >= 64) materialIdx = 63;

    // Map the UBO slot for this frame (single slot, overwritten per draw)
    auto mapped = rhi::buffer_map(m_MaterialUBOs[frameIndex][materialIdx]);
    GPUMaterialUBO& ubo = *static_cast<GPUMaterialUBO*>(mapped.ptr);

    ubo.AlbedoColor = mat.AlbedoColor;
    ubo.Metallic    = mat.Metallic;
    ubo.Roughness   = mat.Roughness;
    ubo.AO          = mat.AO;

    ubo.HasAlbedoMap            = mat.AlbedoMap            ? 1.0f : 0.0f;
    ubo.HasNormalMap            = mat.NormalMap            ? 1.0f : 0.0f;
    ubo.HasMetallicRoughnessMap = mat.MetallicRoughnessMap ? 1.0f : 0.0f;
    ubo.HasAOMap                = mat.AOMap                ? 1.0f : 0.0f;
    rhi::buffer_unmap(m_MaterialUBOs[frameIndex][materialIdx]);

    rhi::DescriptorSet set = m_MaterialSets[frameIndex][materialIdx];
    rhi::Texture texAlbedo = mat.AlbedoMap            ? mat.AlbedoMap            : m_WhiteTex;
    rhi::Texture texNormal = mat.NormalMap            ? mat.NormalMap            : m_WhiteTex;
    rhi::Texture texMR     = mat.MetallicRoughnessMap ? mat.MetallicRoughnessMap : m_WhiteTex;
    rhi::Texture texAO     = mat.AOMap                ? mat.AOMap                : m_WhiteTex;

    rhi::DescriptorWrite writes[5] = {
        {.binding=0, .type=rhi::DescriptorType::CombinedImageSampler, .texture={texAlbedo, m_LinearSampler}},
        {.binding=1, .type=rhi::DescriptorType::CombinedImageSampler, .texture={texNormal, m_LinearSampler}},
        {.binding=2, .type=rhi::DescriptorType::CombinedImageSampler, .texture={texMR,     m_LinearSampler}},
        {.binding=3, .type=rhi::DescriptorType::CombinedImageSampler, .texture={texAO,     m_LinearSampler}},
        {.binding=4, .type=rhi::DescriptorType::UniformBuffer,        .buffer ={m_MaterialUBOs[frameIndex][materialIdx], 0, sizeof(GPUMaterialUBO)}}
    };

    rhi::descriptor_set_write(set, writes, 5);
    rhi::bind_descriptor_set(cmd, set, 0);
}

// ================================================================
//  Shadow Pass
// ================================================================
void SceneRenderer::ShadowPass(const rhi::CmdBuf& cmd, const Ref<Scene>& scene,
                                const glm::mat4& lightSpaceMatrix)
{
    rhi::begin_region(cmd, "ShadowPass", 0.8f, 0.6f, 0.2f);

    rhi::TextureBarrier bar{
        .tex=m_ShadowMap,
        .old_layout=rhi::TextureLayout::Undefined,.new_layout=rhi::TextureLayout::DepthStencilTarget,
        .src_stage=rhi::PipelineStage::Top,.dst_stage=rhi::PipelineStage::EarlyDepth|rhi::PipelineStage::LateDepth,
        .src_access=rhi::Access::None,.dst_access=rhi::Access::DepthWrite,
        .base_mip=0,.mip_count=1,.base_layer=0,.layer_count=1
    };
    rhi::texture_barrier(cmd, &bar, 1);

    rhi::begin_render_pass(cmd, {
        .depth={ .texture=m_ShadowMap, .load_op=rhi::LoadOp::Clear, .store_op=rhi::StoreOp::Store,
                 .clear={1.0f,0} },
        .has_depth=true
    });

    rhi::set_viewport(cmd, {0,0,(float)m_ShadowRes,(float)m_ShadowRes,0,1});
    rhi::set_scissor (cmd, {0,0,m_ShadowRes,m_ShadowRes});
    rhi::bind_pipeline(cmd, m_ShadowPipeline);

    auto meshView = scene->GetRegistry().view<TransformComponent, MeshComponent>();
    for (auto entity : meshView)
    {
        auto [tc, mc] = meshView.get<TransformComponent, MeshComponent>(entity);
        if (!mc.CastShadow || !mc.VertexBuffer) continue;

        ShadowPushConstants pushConst{ lightSpaceMatrix, tc.GetTransform() };
        rhi::push_constants_raw(cmd, rhi::ShaderStage::Vertex, 0, sizeof(pushConst), &pushConst);
        rhi::bind_vertex_buffer(cmd, mc.VertexBuffer, 0);
        rhi::bind_index_buffer(cmd, mc.IndexBuffer, 0, rhi::IndexType::Uint32);
        rhi::draw_indexed(cmd, {.index_count=mc.IndexCount});
    }

    rhi::end_render_pass(cmd);

    rhi::TextureBarrier barSample{
        .tex=m_ShadowMap,
        .old_layout=rhi::TextureLayout::DepthStencilTarget,.new_layout=rhi::TextureLayout::ShaderReadOnly,
        .src_stage=rhi::PipelineStage::LateDepth,.dst_stage=rhi::PipelineStage::Fragment,
        .src_access=rhi::Access::DepthWrite,.dst_access=rhi::Access::ShaderRead,
        .base_mip=0,.mip_count=1,.base_layer=0,.layer_count=1
    };
    rhi::texture_barrier(cmd, &barSample, 1);
    rhi::end_region(cmd);
}

// ================================================================
//  Geometry Pass
// ================================================================
void SceneRenderer::GeometryPass(const rhi::CmdBuf& cmd, const Ref<Scene>& scene,
                                  const glm::mat4& view, const glm::mat4& proj,
                                  const RenderSettings& settings)
{
    rhi::begin_region(cmd, "GeometryPass", 0.2f, 0.8f, 0.4f);

    rhi::TextureBarrier bars[6];
    auto mkBar = [](rhi::Texture t, rhi::TextureLayout newL, rhi::Access dstA, rhi::PipelineStage dstS) {
        return rhi::TextureBarrier{
            .tex=t,.old_layout=rhi::TextureLayout::Undefined,.new_layout=newL,
            .src_stage=rhi::PipelineStage::Top,.dst_stage=dstS,
            .src_access=rhi::Access::None,.dst_access=dstA,
            .base_mip=0,.mip_count=1,.base_layer=0,.layer_count=1
        };
    };
    bars[0]=mkBar(m_GPosition,rhi::TextureLayout::ColorTarget,rhi::Access::ColorWrite,rhi::PipelineStage::ColorOutput);
    bars[1]=mkBar(m_GNormal,  rhi::TextureLayout::ColorTarget,rhi::Access::ColorWrite,rhi::PipelineStage::ColorOutput);
    bars[2]=mkBar(m_GAlbedo,  rhi::TextureLayout::ColorTarget,rhi::Access::ColorWrite,rhi::PipelineStage::ColorOutput);
    bars[3]=mkBar(m_GPBR,     rhi::TextureLayout::ColorTarget,rhi::Access::ColorWrite,rhi::PipelineStage::ColorOutput);
    bars[4]=mkBar(m_GEntityID,rhi::TextureLayout::ColorTarget,rhi::Access::ColorWrite,rhi::PipelineStage::ColorOutput);
    bars[5]=mkBar(m_GDepth,   rhi::TextureLayout::DepthStencilTarget,rhi::Access::DepthWrite,rhi::PipelineStage::EarlyDepth|rhi::PipelineStage::LateDepth);
    rhi::texture_barrier(cmd, bars, 6);

    float clearEntityFloat;
    int clearEntityInt = -1;
    memcpy(&clearEntityFloat, &clearEntityInt, sizeof(float));

    rhi::begin_render_pass(cmd, {
        .color = {
            {.texture=m_GPosition,.load_op=rhi::LoadOp::Clear,.store_op=rhi::StoreOp::Store,.clear={0,0,0,1}},
            {.texture=m_GNormal,  .load_op=rhi::LoadOp::Clear,.store_op=rhi::StoreOp::Store,.clear={0,0,0,1}},
            {.texture=m_GAlbedo,  .load_op=rhi::LoadOp::Clear,.store_op=rhi::StoreOp::Store,.clear={0,0,0,1}},
            {.texture=m_GPBR,     .load_op=rhi::LoadOp::Clear,.store_op=rhi::StoreOp::Store,.clear={0,0,0,1}},
            {.texture=m_GEntityID,.load_op=rhi::LoadOp::Clear,.store_op=rhi::StoreOp::Store,.clear={clearEntityFloat,0,0,0}},
        },
        .color_count=5,
        .depth={.texture=m_GDepth,.load_op=rhi::LoadOp::Clear,.store_op=rhi::StoreOp::Store,.clear={1.0f,0}},
        .has_depth=true
    });

    rhi::set_viewport(cmd, {0,0,(float)m_Width,(float)m_Height,0,1});
    rhi::set_scissor (cmd, {0,0,m_Width,m_Height});

    rhi::bind_pipeline(cmd, settings.EnableWireframe ? m_GeometryWireframePipeline : m_GeometryPipeline);

    glm::mat4 viewProj = proj * view;

    uint32_t materialIdx = 0;
    auto meshView = scene->GetRegistry().view<TransformComponent, MeshComponent>();
    for (auto entity : meshView)
    {
        auto [tc, mc] = meshView.get<TransformComponent, MeshComponent>(entity);
        if (!mc.VertexBuffer) continue;

        MaterialComponent defaultMat;
        MaterialComponent* mat = &defaultMat;
        if (scene->GetRegistry().all_of<MaterialComponent>(entity))
            mat = &scene->GetRegistry().get<MaterialComponent>(entity);

        BindMaterial(cmd, *mat, 0, materialIdx); // Using frameIndex 0 for now as it's hardcoded elsewhere
        
        GeometryPushConstants pushConst{ tc.GetTransform(), viewProj, (int)(uint32_t)entity };
        rhi::push_constants_raw(cmd, rhi::ShaderStage::Vertex, 0, sizeof(pushConst), &pushConst);

        rhi::bind_vertex_buffer(cmd, mc.VertexBuffer, 0);
        rhi::bind_index_buffer(cmd, mc.IndexBuffer, 0, rhi::IndexType::Uint32);
        rhi::draw_indexed(cmd, {.index_count=mc.IndexCount});
        
        materialIdx++;
    }

    rhi::end_render_pass(cmd);

    rhi::TextureBarrier readBars[4];
    auto mkRead = [](rhi::Texture t) {
        return rhi::TextureBarrier{
            .tex=t,.old_layout=rhi::TextureLayout::ColorTarget,.new_layout=rhi::TextureLayout::ShaderReadOnly,
            .src_stage=rhi::PipelineStage::ColorOutput,.dst_stage=rhi::PipelineStage::Fragment,
            .src_access=rhi::Access::ColorWrite,.dst_access=rhi::Access::ShaderRead,
            .base_mip=0,.mip_count=1,.base_layer=0,.layer_count=1
        };
    };
    readBars[0]=mkRead(m_GPosition);
    readBars[1]=mkRead(m_GNormal);
    readBars[2]=mkRead(m_GAlbedo);
    readBars[3]=mkRead(m_GPBR);
    rhi::texture_barrier(cmd, readBars, 4);
    rhi::end_region(cmd);
}

// ================================================================
//  Lighting Pass
// ================================================================
void SceneRenderer::LightingPass(const rhi::CmdBuf& cmd, const Ref<Scene>& scene,
                                  const glm::vec3& cameraPos,
                                  const glm::mat4& lightSpaceMatrix,
                                  const RenderSettings& settings)
{
    (void)scene; (void)cameraPos; (void)lightSpaceMatrix; (void)settings;
    rhi::begin_region(cmd, "LightingPass", 0.3f, 0.4f, 0.9f);

    rhi::TextureBarrier bar{
        .tex=m_LightPassColor,
        .old_layout=rhi::TextureLayout::Undefined,.new_layout=rhi::TextureLayout::ColorTarget,
        .src_stage=rhi::PipelineStage::Top,.dst_stage=rhi::PipelineStage::ColorOutput,
        .src_access=rhi::Access::None,.dst_access=rhi::Access::ColorWrite,
        .base_mip=0,.mip_count=1,.base_layer=0,.layer_count=1
    };
    rhi::texture_barrier(cmd, &bar, 1);

    rhi::begin_render_pass(cmd, {
        .color={{.texture=m_LightPassColor,.load_op=rhi::LoadOp::Clear,.store_op=rhi::StoreOp::Store,.clear={0,0,0,1}}},
        .color_count=1
    });

    rhi::set_viewport(cmd, {0,0,(float)m_Width,(float)m_Height,0,1});
    rhi::set_scissor (cmd, {0,0,m_Width,m_Height});
    rhi::bind_pipeline(cmd, m_LightingPipeline);

    rhi::DescriptorWrite writes[6] = {
        {.binding=0,.type=rhi::DescriptorType::CombinedImageSampler, .texture={m_GPosition, m_LinearSampler}},
        {.binding=1,.type=rhi::DescriptorType::CombinedImageSampler, .texture={m_GNormal,   m_LinearSampler}},
        {.binding=2,.type=rhi::DescriptorType::CombinedImageSampler, .texture={m_GAlbedo,   m_LinearSampler}},
        {.binding=3,.type=rhi::DescriptorType::CombinedImageSampler, .texture={m_GPBR,      m_LinearSampler}},
        {.binding=4,.type=rhi::DescriptorType::CombinedImageSampler, .texture={m_ShadowMap, m_ShadowSampler}},
        {.binding=5,.type=rhi::DescriptorType::UniformBuffer,        .buffer ={m_LightUBOs[0], 0, sizeof(GPULightUBO)}}
    };
    rhi::descriptor_set_write(m_LightingSet, writes, 6);
    rhi::bind_descriptor_set(cmd, m_LightingSet);

    rhi::draw(cmd, {.vertex_count=3});

    rhi::end_render_pass(cmd);

    rhi::TextureBarrier finalBar{
        .tex=m_LightPassColor,
        .old_layout=rhi::TextureLayout::ColorTarget,.new_layout=rhi::TextureLayout::ShaderReadOnly,
        .src_stage=rhi::PipelineStage::ColorOutput,.dst_stage=rhi::PipelineStage::Fragment,
        .src_access=rhi::Access::ColorWrite,.dst_access=rhi::Access::ShaderRead,
        .base_mip=0,.mip_count=1,.base_layer=0,.layer_count=1
    };
    rhi::texture_barrier(cmd, &finalBar, 1);
    rhi::end_region(cmd);
}

// ================================================================
//  RenderScene  (main entry point)
// ================================================================
void SceneRenderer::RenderScene(const Ref<Scene>& scene, const EditorCamera& camera,
                                 const RenderSettings& settings)
{
    // Lazy load GPU resources
    auto meshView = scene->GetRegistry().view<MeshComponent>();
    for (auto entity : meshView)
    {
        auto& mc = meshView.get<MeshComponent>(entity);
        //ENGINE_INFO("RenderScene: Mesh entity found. ModelPath='{0}'", mc.ModelPath);
        if (!mc.VertexBuffer && !mc.ModelPath.empty())
        {
            Ref<Model> model = AssetManager::GetModel(mc.ModelPath);
            if (model && !model->m_meshes.empty())
            {
                std::vector<Vertex> all_vertices;
                std::vector<uint32_t> all_indices;
                for (auto& mesh : model->m_meshes)
                {
                    uint32_t base_index = all_vertices.size();
                    all_vertices.insert(all_vertices.end(), mesh->m_vertices.begin(), mesh->m_vertices.end());
                    for (uint32_t idx : mesh->m_indices)
                        all_indices.push_back(base_index + idx);
                }
                mc.VertexCount = all_vertices.size();
                mc.IndexCount = all_indices.size();
                if (mc.VertexCount > 0 && mc.IndexCount > 0)
                {
                    mc.VertexBuffer = rhi::buffer_create({
                        .size = (uint32_t)(mc.VertexCount * sizeof(Vertex)),
                        .usage = rhi::BufferUsage::Vertex,
                        .memory = rhi::MemoryType::CpuToGpu,
                        .name = "MeshVB"
                    });
                    mc.IndexBuffer = rhi::buffer_create({
                        .size = (uint32_t)(mc.IndexCount * sizeof(uint32_t)),
                        .usage = rhi::BufferUsage::Index,
                        .memory = rhi::MemoryType::CpuToGpu,
                        .name = "MeshIB"
                    });
                    
                    auto vmap = rhi::buffer_map(mc.VertexBuffer);
                    memcpy(vmap.ptr, all_vertices.data(), mc.VertexCount * sizeof(Vertex));
                    rhi::buffer_unmap(mc.VertexBuffer);

                    auto imap = rhi::buffer_map(mc.IndexBuffer);
                    memcpy(imap.ptr, all_indices.data(), mc.IndexCount * sizeof(uint32_t));
                    rhi::buffer_unmap(mc.IndexBuffer);
                }
            }
        }
    }

    auto matView = scene->GetRegistry().view<MaterialComponent>();
    for (auto entity : matView)
    {
        auto& mat = matView.get<MaterialComponent>(entity);
        if (!mat.AlbedoMap && !mat.AlbedoMapPath.empty())
            mat.AlbedoMap = AssetManager::GetTexture(mat.AlbedoMapPath);
        if (!mat.NormalMap && !mat.NormalMapPath.empty())
            mat.NormalMap = AssetManager::GetTexture(mat.NormalMapPath);
        if (!mat.MetallicRoughnessMap && !mat.MetallicRoughnessMapPath.empty())
            mat.MetallicRoughnessMap = AssetManager::GetTexture(mat.MetallicRoughnessMapPath);
        if (!mat.AOMap && !mat.AOMapPath.empty())
            mat.AOMap = AssetManager::GetTexture(mat.AOMapPath);
    }

    // Compute light-space matrix from first shadow-casting directional light
    glm::mat4 lightSpaceMatrix(1.0f);
    {
        auto lv = scene->GetRegistry().view<TransformComponent, LightComponent>();
        for (auto e : lv)
        {
            auto [tc, lc] = lv.get<TransformComponent, LightComponent>(e);
            if (lc.Type == LightType::Directional && lc.CastShadows)
            {
                glm::vec3 dir = glm::normalize(glm::vec3(
                    glm::sin(tc.Rotation.y)*glm::cos(tc.Rotation.x),
                   -glm::sin(tc.Rotation.x),
                   -glm::cos(tc.Rotation.y)*glm::cos(tc.Rotation.x)));
                lightSpaceMatrix = CalcLightSpaceMatrix(dir);
                break;
            }
        }
    }

    UploadLightUBO(scene, camera.GetPosition(), lightSpaceMatrix, 0, settings);

    rhi::cmdbuf_reset(m_Cmd);
    rhi::cmdbuf_begin(m_Cmd);

    if (settings.EnableShadows)
    {
        ShadowPass(m_Cmd, scene, lightSpaceMatrix);
    }
    else
    {
        rhi::TextureBarrier dummyBar{
            .tex=m_ShadowMap,
            .old_layout=rhi::TextureLayout::Undefined,.new_layout=rhi::TextureLayout::ShaderReadOnly,
            .src_stage=rhi::PipelineStage::Top,.dst_stage=rhi::PipelineStage::Fragment,
            .src_access=rhi::Access::None,.dst_access=rhi::Access::ShaderRead,
            .base_mip=0,.mip_count=1,.base_layer=0,.layer_count=1
        };
        rhi::texture_barrier(m_Cmd, &dummyBar, 1);
    }

    GeometryPass(m_Cmd, scene, camera.GetViewMatrix(), camera.GetProjectionMatrix(), settings);
    LightingPass(m_Cmd, scene, camera.GetPosition(), lightSpaceMatrix, settings);
    
    // ----- Outline Pass -----
    if (m_SelectedEntity && m_SelectedEntity.HasComponent<MeshComponent>())
    {
        rhi::begin_region(m_Cmd, "OutlinePass", 0.1f, 0.9f, 0.2f);
        
        // Transition LightPassColor back to ColorTarget
        rhi::TextureBarrier outBar{
            .tex=m_LightPassColor,
            .old_layout=rhi::TextureLayout::ShaderReadOnly,.new_layout=rhi::TextureLayout::ColorTarget,
            .src_stage=rhi::PipelineStage::Fragment,.dst_stage=rhi::PipelineStage::ColorOutput,
            .src_access=rhi::Access::ShaderRead,.dst_access=rhi::Access::ColorWrite,
            .base_mip=0,.mip_count=1,.base_layer=0,.layer_count=1
        };
        rhi::texture_barrier(m_Cmd, &outBar, 1);

        rhi::begin_render_pass(m_Cmd, {
            .color = {
                {.texture=m_LightPassColor,.load_op=rhi::LoadOp::Load,.store_op=rhi::StoreOp::Store}
            },
            .color_count=1,
            // Re-use G-Buffer depth to depth-test the outline!
            .depth={.texture=m_GDepth,.load_op=rhi::LoadOp::Load,.store_op=rhi::StoreOp::Store},
            .has_depth=true
        });

        rhi::set_viewport(m_Cmd, {0,0,(float)m_Width,(float)m_Height,0,1});
        rhi::set_scissor (m_Cmd, {0,0,m_Width,m_Height});
        rhi::bind_pipeline(m_Cmd, m_OutlinePipeline);

        auto& tc = m_SelectedEntity.GetComponent<TransformComponent>();
        auto& mc = m_SelectedEntity.GetComponent<MeshComponent>();

        if (mc.VertexBuffer)
        {
            glm::mat4 viewProj = camera.GetProjectionMatrix() * camera.GetViewMatrix();
            
            struct OutlinePushConstants {
                glm::mat4 Model;
                glm::mat4 ViewProj;
                glm::vec4 Color;
            };
            OutlinePushConstants pc{ tc.GetTransform(), viewProj, settings.OutlineColor };
            rhi::push_constants_raw(m_Cmd, rhi::ShaderStage::Vertex | rhi::ShaderStage::Fragment, 0, sizeof(pc), &pc);

            rhi::bind_vertex_buffer(m_Cmd, mc.VertexBuffer, 0);
            rhi::bind_index_buffer(m_Cmd, mc.IndexBuffer, 0, rhi::IndexType::Uint32);
            rhi::draw_indexed(m_Cmd, {.index_count=mc.IndexCount});
        }

        rhi::end_render_pass(m_Cmd);
        
        // Transition back to ShaderReadOnly
        rhi::TextureBarrier finalBar{
            .tex=m_LightPassColor,
            .old_layout=rhi::TextureLayout::ColorTarget,.new_layout=rhi::TextureLayout::ShaderReadOnly,
            .src_stage=rhi::PipelineStage::ColorOutput,.dst_stage=rhi::PipelineStage::Fragment,
            .src_access=rhi::Access::ColorWrite,.dst_access=rhi::Access::ShaderRead,
            .base_mip=0,.mip_count=1,.base_layer=0,.layer_count=1
        };
        rhi::texture_barrier(m_Cmd, &finalBar, 1);

        rhi::end_region(m_Cmd);
    }

    rhi::cmdbuf_end(m_Cmd);
    rhi::queue_submit(&m_Cmd, 1, nullptr, 0, nullptr, 0, m_Fence);
    rhi::fence_wait(m_Fence);
    rhi::fence_reset(m_Fence);
}

} // namespace Engine
