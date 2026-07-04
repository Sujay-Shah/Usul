// =============================================================
//  vkrhi.cpp — Vulkan 1.3 backend (production-ready)
//
//  Requirements
//  ------------
//    Vulkan 1.3 (dynamic rendering, synchronisation2)
//    VMA 3.x   (vk_mem_alloc.h) — define VMA_IMPLEMENTATION here
//    C++20
//
//  Design
//  ------
//    • All sub-allocations via VMA: no per-resource vkAllocateMemory
//    • Synchronisation via VK_KHR_synchronization2 (core in VK 1.3)
//    • Dynamic rendering (no VkRenderPass objects, no VkFramebuffer)
//    • Push constants issued with the pipeline layout from PipelineSlot
//    • Debug names forwarded to VK_EXT_debug_utils when present
//    • Shutdown for_each() calls catch un-destroyed resources
//    • Pool<> generation counters catch all stale-handle use-after-free
// =============================================================

#define VMA_IMPLEMENTATION
#include "vkrhi_internal.hpp"
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <algorithm>
#include <GLFW/glfw3.h>
#ifdef _WIN32
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#endif
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.cpp"


namespace vkrhi {

// =============================================================
//  GLOBALS
// =============================================================

Ctx g_ctx;

// Large pools MUST be static — never stack-allocate 1M-entry arrays.
static rhi::Pool<BufferSlot,     rhi::MAX_BUFFERS,         rhi::Buffer>           s_buffers;
static rhi::Pool<TextureSlot,    rhi::MAX_TEXTURES,        rhi::Texture>          s_textures;
static rhi::Pool<SamplerSlot,    rhi::MAX_SAMPLERS,        rhi::Sampler>          s_samplers;
static rhi::Pool<ShaderSlot,     rhi::MAX_SHADERS,         rhi::Shader>           s_shaders;
static rhi::Pool<DescLayoutSlot, rhi::MAX_DESC_LAYOUTS,    rhi::DescriptorLayout> s_desc_layouts;
static rhi::Pool<DescSetSlot,    rhi::MAX_DESCRIPTOR_SETS, rhi::DescriptorSet>    s_desc_sets;
static rhi::Pool<PipelineSlot,   rhi::MAX_PIPELINES,       rhi::Pipeline>         s_pipelines;
static rhi::Pool<CmdBufSlot,     rhi::MAX_CMDBUFS,         rhi::CmdBuf>           s_cmdbufs;
static rhi::Pool<FenceSlot,      rhi::MAX_FENCES,          rhi::Fence>            s_fences;
static rhi::Pool<SemaphoreSlot,  rhi::MAX_SEMAPHORES,      rhi::Semaphore>        s_semaphores;

// Export references so the extern declarations in the header compile
rhi::Pool<BufferSlot,     rhi::MAX_BUFFERS,         rhi::Buffer>&           g_buffers      = s_buffers;
rhi::Pool<TextureSlot,    rhi::MAX_TEXTURES,        rhi::Texture>&          g_textures     = s_textures;
rhi::Pool<SamplerSlot,    rhi::MAX_SAMPLERS,        rhi::Sampler>&          g_samplers     = s_samplers;
rhi::Pool<ShaderSlot,     rhi::MAX_SHADERS,         rhi::Shader>&           g_shaders      = s_shaders;
rhi::Pool<DescLayoutSlot, rhi::MAX_DESC_LAYOUTS,    rhi::DescriptorLayout>& g_desc_layouts = s_desc_layouts;
rhi::Pool<DescSetSlot,    rhi::MAX_DESCRIPTOR_SETS, rhi::DescriptorSet>&    g_desc_sets    = s_desc_sets;
rhi::Pool<PipelineSlot,   rhi::MAX_PIPELINES,       rhi::Pipeline>&         g_pipelines    = s_pipelines;
rhi::Pool<CmdBufSlot,     rhi::MAX_CMDBUFS,         rhi::CmdBuf>&           g_cmdbufs      = s_cmdbufs;
rhi::Pool<FenceSlot,      rhi::MAX_FENCES,          rhi::Fence>&            g_fences       = s_fences;
rhi::Pool<SemaphoreSlot,  rhi::MAX_SEMAPHORES,      rhi::Semaphore>&        g_semaphores   = s_semaphores;

// =============================================================
//  CONVERSION TABLES
// =============================================================

VkFormat to_vk_format(rhi::Format f) noexcept
{
    static constexpr VkFormat tbl[] = {
        VK_FORMAT_UNDEFINED,
        VK_FORMAT_R8_UNORM, VK_FORMAT_R8_SNORM, VK_FORMAT_R8_UINT, VK_FORMAT_R8_SINT,
        VK_FORMAT_R8G8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_R16_SFLOAT, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R16_UINT, VK_FORMAT_R16_SINT,
        VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_FORMAT_R32_UINT, VK_FORMAT_R32G32_UINT, VK_FORMAT_R32G32B32A32_UINT,
        VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_FORMAT_B10G11R11_UFLOAT_PACK32, VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,
        VK_FORMAT_D16_UNORM, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_BC1_RGBA_UNORM_BLOCK, VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
        VK_FORMAT_BC2_UNORM_BLOCK,      VK_FORMAT_BC2_SRGB_BLOCK,
        VK_FORMAT_BC3_UNORM_BLOCK,      VK_FORMAT_BC3_SRGB_BLOCK,
        VK_FORMAT_BC4_UNORM_BLOCK,      VK_FORMAT_BC4_SNORM_BLOCK,
        VK_FORMAT_BC5_UNORM_BLOCK,      VK_FORMAT_BC5_SNORM_BLOCK,
        VK_FORMAT_BC6H_UFLOAT_BLOCK,    VK_FORMAT_BC6H_SFLOAT_BLOCK,
        VK_FORMAT_BC7_UNORM_BLOCK,      VK_FORMAT_BC7_SRGB_BLOCK,
    };
    static_assert(std::size(tbl) == static_cast<u32>(rhi::Format::Count));
    const u32 idx = static_cast<u32>(f);
    return idx < static_cast<u32>(rhi::Format::Count) ? tbl[idx] : VK_FORMAT_UNDEFINED;
}

VkImageLayout to_vk_layout(rhi::TextureLayout l) noexcept
{
    switch (l)
    {
        case rhi::TextureLayout::Undefined:            return VK_IMAGE_LAYOUT_UNDEFINED;
        case rhi::TextureLayout::General:              return VK_IMAGE_LAYOUT_GENERAL;
        case rhi::TextureLayout::ColorTarget:          return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        case rhi::TextureLayout::DepthStencilTarget:   return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        case rhi::TextureLayout::DepthStencilReadOnly: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        case rhi::TextureLayout::ShaderReadOnly:       return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        case rhi::TextureLayout::TransferSrc:          return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        case rhi::TextureLayout::TransferDst:          return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        case rhi::TextureLayout::Present:              return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        default:                                       return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

VkAccessFlags2 to_vk_access2(rhi::Access a) noexcept
{
    VkAccessFlags2 out = VK_ACCESS_2_NONE;
    const u32 u = static_cast<u32>(a);
    if (u & static_cast<u32>(rhi::Access::IndirectRead))  out |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
    if (u & static_cast<u32>(rhi::Access::IndexRead))     out |= VK_ACCESS_2_INDEX_READ_BIT;
    if (u & static_cast<u32>(rhi::Access::VertexRead))    out |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
    if (u & static_cast<u32>(rhi::Access::UniformRead))   out |= VK_ACCESS_2_UNIFORM_READ_BIT;
    if (u & static_cast<u32>(rhi::Access::ShaderRead))    out |= VK_ACCESS_2_SHADER_READ_BIT;
    if (u & static_cast<u32>(rhi::Access::ShaderWrite))   out |= VK_ACCESS_2_SHADER_WRITE_BIT;
    if (u & static_cast<u32>(rhi::Access::ColorRead))     out |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
    if (u & static_cast<u32>(rhi::Access::ColorWrite))    out |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    if (u & static_cast<u32>(rhi::Access::DepthRead))     out |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    if (u & static_cast<u32>(rhi::Access::DepthWrite))    out |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    if (u & static_cast<u32>(rhi::Access::TransferRead))  out |= VK_ACCESS_2_TRANSFER_READ_BIT;
    if (u & static_cast<u32>(rhi::Access::TransferWrite)) out |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
    if (u & static_cast<u32>(rhi::Access::MemoryRead))    out |= VK_ACCESS_2_MEMORY_READ_BIT;
    if (u & static_cast<u32>(rhi::Access::MemoryWrite))   out |= VK_ACCESS_2_MEMORY_WRITE_BIT;
    return out;
}

VkPipelineStageFlags2 to_vk_stage2(rhi::PipelineStage s) noexcept
{
    VkPipelineStageFlags2 out = VK_PIPELINE_STAGE_2_NONE;
    const u32 u = static_cast<u32>(s);
    if (u & static_cast<u32>(rhi::PipelineStage::Top))         out |= VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    if (u & static_cast<u32>(rhi::PipelineStage::DrawIndirect))out |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
    if (u & static_cast<u32>(rhi::PipelineStage::Vertex))      out |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    if (u & static_cast<u32>(rhi::PipelineStage::EarlyDepth))  out |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
    if (u & static_cast<u32>(rhi::PipelineStage::Fragment))    out |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    if (u & static_cast<u32>(rhi::PipelineStage::LateDepth))   out |= VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
    if (u & static_cast<u32>(rhi::PipelineStage::ColorOutput)) out |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    if (u & static_cast<u32>(rhi::PipelineStage::Compute))     out |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    if (u & static_cast<u32>(rhi::PipelineStage::Transfer))    out |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    if (u & static_cast<u32>(rhi::PipelineStage::Bottom))      out |= VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    return out ? out : VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
}

VkShaderStageFlags to_vk_shader_stage(rhi::ShaderStage s) noexcept
{
    if (s == rhi::ShaderStage::All) return VK_SHADER_STAGE_ALL;
    VkShaderStageFlags out = 0;
    if (has(s, rhi::ShaderStage::Vertex))   out |= VK_SHADER_STAGE_VERTEX_BIT;
    if (has(s, rhi::ShaderStage::Fragment)) out |= VK_SHADER_STAGE_FRAGMENT_BIT;
    if (has(s, rhi::ShaderStage::Compute))  out |= VK_SHADER_STAGE_COMPUTE_BIT;
    return out;
}

VkDescriptorType to_vk_desc_type(rhi::DescriptorType t) noexcept
{
    switch (t)
    {
        case rhi::DescriptorType::UniformBuffer:        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case rhi::DescriptorType::UniformBufferDynamic: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        case rhi::DescriptorType::StorageBuffer:        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case rhi::DescriptorType::StorageBufferDynamic: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        case rhi::DescriptorType::SampledTexture:       return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case rhi::DescriptorType::StorageTexture:       return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case rhi::DescriptorType::Sampler:              return VK_DESCRIPTOR_TYPE_SAMPLER;
        case rhi::DescriptorType::CombinedImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case rhi::DescriptorType::InputAttachment:      return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        default:                                        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
}

VkCompareOp to_vk_compare_op(rhi::CompareOp c) noexcept
{
    static constexpr VkCompareOp tbl[] = {
        VK_COMPARE_OP_NEVER, VK_COMPARE_OP_LESS, VK_COMPARE_OP_EQUAL,
        VK_COMPARE_OP_LESS_OR_EQUAL, VK_COMPARE_OP_GREATER, VK_COMPARE_OP_NOT_EQUAL,
        VK_COMPARE_OP_GREATER_OR_EQUAL, VK_COMPARE_OP_ALWAYS
    };
    return tbl[static_cast<u32>(c)];
}

VkFilter             to_vk_filter  (rhi::SamplerFilter f) noexcept { return f == rhi::SamplerFilter::Linear  ? VK_FILTER_LINEAR            : VK_FILTER_NEAREST; }
VkSamplerMipmapMode  to_vk_mipmap  (rhi::SamplerMipmap m) noexcept { return m == rhi::SamplerMipmap::Linear  ? VK_SAMPLER_MIPMAP_MODE_LINEAR: VK_SAMPLER_MIPMAP_MODE_NEAREST; }
VkCullModeFlagBits   to_vk_cull    (rhi::CullMode c) noexcept
{
    switch (c)
    {
        case rhi::CullMode::None:  return VK_CULL_MODE_NONE;
        case rhi::CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
        case rhi::CullMode::Back:  return VK_CULL_MODE_BACK_BIT;
        default:                   return VK_CULL_MODE_BACK_BIT;
    }
}
VkFrontFace to_vk_front_face(rhi::FrontFace f) noexcept
    { return f == rhi::FrontFace::CCW ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE; }
VkPolygonMode to_vk_fill(rhi::FillMode f) noexcept
    { return f == rhi::FillMode::Wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL; }
VkPrimitiveTopology to_vk_topology(rhi::PrimitiveTopology t) noexcept
{
    switch (t)
    {
        case rhi::PrimitiveTopology::TriangleList:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case rhi::PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case rhi::PrimitiveTopology::LineList:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case rhi::PrimitiveTopology::PointList:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        default:                                    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}
VkSamplerAddressMode to_vk_address(rhi::SamplerAddress a) noexcept
{
    switch (a)
    {
        case rhi::SamplerAddress::Repeat:              return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case rhi::SamplerAddress::MirroredRepeat:      return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case rhi::SamplerAddress::ClampToEdge:         return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case rhi::SamplerAddress::ClampToBorder:       return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case rhi::SamplerAddress::MirrorClampToEdge:   return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        default:                                       return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}
VkBlendFactor to_vk_blend_factor(rhi::BlendFactor b) noexcept
{
    static constexpr VkBlendFactor tbl[] = {
        VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
        VK_BLEND_FACTOR_DST_COLOR, VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
        VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        VK_BLEND_FACTOR_DST_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
        VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
        VK_BLEND_FACTOR_CONSTANT_COLOR, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
    };
    return tbl[static_cast<u32>(b)];
}
VkBlendOp to_vk_blend_op(rhi::BlendOp o) noexcept
{
    static constexpr VkBlendOp tbl[] = {
        VK_BLEND_OP_ADD, VK_BLEND_OP_SUBTRACT, VK_BLEND_OP_REVERSE_SUBTRACT,
        VK_BLEND_OP_MIN, VK_BLEND_OP_MAX
    };
    return tbl[static_cast<u32>(o)];
}
VmaMemoryUsage to_vma_usage(rhi::MemoryType m) noexcept
{
    switch (m)
    {
        case rhi::MemoryType::GpuOnly:     return VMA_MEMORY_USAGE_GPU_ONLY;
        case rhi::MemoryType::CpuToGpu:    return VMA_MEMORY_USAGE_CPU_TO_GPU;
        case rhi::MemoryType::GpuToCpu:    return VMA_MEMORY_USAGE_GPU_TO_CPU;
        case rhi::MemoryType::CpuCoherent: return VMA_MEMORY_USAGE_CPU_ONLY;
        default:                           return VMA_MEMORY_USAGE_GPU_ONLY;
    }
}
VkImageAspectFlags aspect_for_format(VkFormat f) noexcept
{
    switch (f)
    {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:         return VK_IMAGE_ASPECT_DEPTH_BIT;
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        default:                            return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

// =============================================================
//  DEBUG HELPERS
// =============================================================

void set_debug_name(VkObjectType type, u64 handle, const char* name) noexcept
{
    if (!name || !g_ctx.pfn_set_name) return;
    const VkDebugUtilsObjectNameInfoEXT info{
        .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType   = type,
        .objectHandle = handle,
        .pObjectName  = name,
    };
    g_ctx.pfn_set_name(g_ctx.device, &info);
}

// =============================================================
//  INIT
// =============================================================

static bool vk_init(const rhi::InitDesc& desc)
{
    // ---- Instance ----
    const char* layers[]     = { "VK_LAYER_KHRONOS_validation" };
    const char* inst_exts[]  = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR) || defined(_WIN32)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XCB_KHR)
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_METAL_EXT)
        VK_EXT_METAL_SURFACE_EXTENSION_NAME,
        "VK_KHR_portability_enumeration",
#endif
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };
    const u32 inst_ext_count = static_cast<u32>(std::size(inst_exts));

    const VkApplicationInfo app_info{
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = desc.app_name,
        .applicationVersion = desc.app_version,
        .pEngineName        = "rhi",
        .engineVersion      = VK_MAKE_VERSION(1,0,0),
        .apiVersion         = VK_API_VERSION_1_3,
    };
    const VkInstanceCreateInfo inst_ci{
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef VK_USE_PLATFORM_METAL_EXT
        .flags                   = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#endif
        .pApplicationInfo        = &app_info,
        .enabledLayerCount       = desc.validation ? 1u : 0u,
        .ppEnabledLayerNames     = desc.validation ? layers : nullptr,
        .enabledExtensionCount   = inst_ext_count,
        .ppEnabledExtensionNames = inst_exts,
    };
    if (vkCreateInstance(&inst_ci, nullptr, &g_ctx.instance) != VK_SUCCESS)
    {
        fprintf(stderr, "[vkrhi] vkCreateInstance failed\n");
        return false;
    }

    // ---- Physical device selection (prefer discrete GPU) ----
    {
        u32 n = 0;
        vkEnumeratePhysicalDevices(g_ctx.instance, &n, nullptr);
        if (n == 0) { fprintf(stderr, "[vkrhi] no Vulkan devices\n"); return false; }
        VkPhysicalDevice devs[16];
        n = std::min(n, 16u);
        vkEnumeratePhysicalDevices(g_ctx.instance, &n, devs);

        g_ctx.phys_dev = devs[0];
        VkPhysicalDeviceProperties best_props{};
        vkGetPhysicalDeviceProperties(devs[0], &best_props);
        for (u32 i = 1; i < n; ++i)
        {
            VkPhysicalDeviceProperties p{};
            vkGetPhysicalDeviceProperties(devs[i], &p);
            if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                best_props.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                g_ctx.phys_dev  = devs[i];
                best_props      = p;
            }
        }
        g_ctx.props = best_props;
        g_ctx.min_ubo_align = static_cast<u32>(
            g_ctx.props.limits.minUniformBufferOffsetAlignment);
    }

    // ---- Queue families ----
    {
        u32 qfn = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(g_ctx.phys_dev, &qfn, nullptr);
        VkQueueFamilyProperties qfps[16];
        qfn = std::min(qfn, 16u);
        vkGetPhysicalDeviceQueueFamilyProperties(g_ctx.phys_dev, &qfn, qfps);

        g_ctx.gfx_family  = ~0u;
        g_ctx.comp_family = ~0u;
        g_ctx.xfr_family  = ~0u;

        for (u32 i = 0; i < qfn; ++i)
        {
            if (qfps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && g_ctx.gfx_family  == ~0u) g_ctx.gfx_family  = i;
            if (qfps[i].queueFlags & VK_QUEUE_COMPUTE_BIT  && g_ctx.comp_family == ~0u) g_ctx.comp_family = i;
            if (qfps[i].queueFlags & VK_QUEUE_TRANSFER_BIT && g_ctx.xfr_family  == ~0u) g_ctx.xfr_family  = i;
        }
        if (g_ctx.gfx_family == ~0u) { fprintf(stderr, "[vkrhi] no graphics queue\n"); return false; }
        if (g_ctx.comp_family == ~0u) g_ctx.comp_family = g_ctx.gfx_family;
        if (g_ctx.xfr_family  == ~0u) g_ctx.xfr_family  = g_ctx.gfx_family;
    }

    // ---- Logical device + synchronization2 + dynamic rendering ----
    {
        static const float prio = 1.f;
        VkDeviceQueueCreateInfo qcis[3];
        u32 qci_count = 0;
        auto add_queue = [&](u32 family)
        {
            for (u32 i = 0; i < qci_count; ++i)
                if (qcis[i].queueFamilyIndex == family) return;
            qcis[qci_count++] = {
                .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = family,
                .queueCount       = 1,
                .pQueuePriorities = &prio,
            };
        };
        add_queue(g_ctx.gfx_family);
        add_queue(g_ctx.comp_family);
        add_queue(g_ctx.xfr_family);

        // Core 1.3 features we use
        VkPhysicalDeviceVulkan13Features feats13{
            .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .synchronization2 = VK_TRUE,
            .dynamicRendering = VK_TRUE,
        };
        VkPhysicalDeviceVulkan12Features feats12{
            .sType                                      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .pNext                                      = &feats13,
            .descriptorIndexing                         = VK_TRUE,
            .shaderSampledImageArrayNonUniformIndexing  = VK_TRUE,
            .runtimeDescriptorArray                     = VK_TRUE,
            .bufferDeviceAddress                        = VK_TRUE,
        };
        VkPhysicalDeviceFeatures2 feats2{
            .sType    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext    = &feats12,
            .features = {
                .fillModeNonSolid  = VK_TRUE,
                .samplerAnisotropy = VK_TRUE,
            },
        };

        const char* dev_exts[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_METAL_EXT
            "VK_KHR_portability_subset",
#endif
        };
        const VkDeviceCreateInfo dev_ci{
            .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext                   = &feats2,
            .queueCreateInfoCount    = qci_count,
            .pQueueCreateInfos       = qcis,
            .enabledExtensionCount   = static_cast<u32>(std::size(dev_exts)),
            .ppEnabledExtensionNames = dev_exts,
        };
        if (vkCreateDevice(g_ctx.phys_dev, &dev_ci, nullptr, &g_ctx.device) != VK_SUCCESS)
        {
            fprintf(stderr, "[vkrhi] vkCreateDevice failed\n");
            return false;
        }

        vkGetDeviceQueue(g_ctx.device, g_ctx.gfx_family,  0, &g_ctx.graphics_queue);
        vkGetDeviceQueue(g_ctx.device, g_ctx.comp_family,  0, &g_ctx.compute_queue);
        vkGetDeviceQueue(g_ctx.device, g_ctx.xfr_family,   0, &g_ctx.transfer_queue);
    }

    // ---- VMA ----
    {
        const VmaAllocatorCreateInfo vma_ci{
            .flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
            .physicalDevice = g_ctx.phys_dev,
            .device         = g_ctx.device,
            .instance       = g_ctx.instance,
            .vulkanApiVersion = VK_API_VERSION_1_3,
        };
        vmaCreateAllocator(&vma_ci, &g_ctx.allocator);
    }

    // ---- Command pools ----
    {
        auto make_pool = [&](u32 family, VkCommandPool& out)
        {
            const VkCommandPoolCreateInfo ci{
                .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = family,
            };
            vkCreateCommandPool(g_ctx.device, &ci, nullptr, &out);
        };
        make_pool(g_ctx.gfx_family,  g_ctx.gfx_pool);
        make_pool(g_ctx.comp_family, g_ctx.comp_pool);
        make_pool(g_ctx.xfr_family,  g_ctx.xfr_pool);
    }

    // ---- Descriptor pool ----
    {
        static constexpr VkDescriptorPoolSize sizes[] = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         4096 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1024 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         2048 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8192 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          4096 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          2048 },
            { VK_DESCRIPTOR_TYPE_SAMPLER,                 512 },
        };
        const VkDescriptorPoolCreateInfo dpi{
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets       = rhi::MAX_DESCRIPTOR_SETS,
            .poolSizeCount = static_cast<u32>(std::size(sizes)),
            .pPoolSizes    = sizes,
        };
        vkCreateDescriptorPool(g_ctx.device, &dpi, nullptr, &g_ctx.desc_pool);
    }

    // ---- Default Pipeline Layout ----
    {
        const VkPushConstantRange pcr{
            .stageFlags = VK_SHADER_STAGE_ALL,
            .offset     = 0,
            .size       = 256,
        };
        const VkPipelineLayoutCreateInfo plci{
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges    = &pcr,
        };
        vkCreatePipelineLayout(g_ctx.device, &plci, nullptr, &g_ctx.default_pipeline_layout);
    }

    // ---- Debug utils ----
    if (desc.validation)
    {
        g_ctx.pfn_begin_label  = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
            vkGetDeviceProcAddr(g_ctx.device, "vkCmdBeginDebugUtilsLabelEXT"));
        g_ctx.pfn_end_label    = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
            vkGetDeviceProcAddr(g_ctx.device, "vkCmdEndDebugUtilsLabelEXT"));
        g_ctx.pfn_insert_label = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(
            vkGetDeviceProcAddr(g_ctx.device, "vkCmdInsertDebugUtilsLabelEXT"));
        g_ctx.pfn_set_name     = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
            vkGetDeviceProcAddr(g_ctx.device, "vkSetDebugUtilsObjectNameEXT"));
        g_ctx.validation_enabled = true;
    }

    printf("[vkrhi] %s  api=1.3  ubo_align=%u\n",
           g_ctx.props.deviceName, g_ctx.min_ubo_align);
    return true;
}

// =============================================================
//  SHUTDOWN
// =============================================================

static void vk_shutdown()
{
    vkDeviceWaitIdle(g_ctx.device);

    // Destroy all leaked resources — for_each catches anything the app forgot
    s_pipelines.for_each([](PipelineSlot& s) {
        vkDestroyPipeline(g_ctx.device, s.pipeline, nullptr);
        // pipeline_layout owned by DescLayoutSlot, not freed here
    });
    s_desc_sets.for_each([](DescSetSlot& s) {
        vkFreeDescriptorSets(g_ctx.device, g_ctx.desc_pool, 1, &s.set);
    });
    s_desc_layouts.for_each([](DescLayoutSlot& s) {
        vkDestroyDescriptorSetLayout(g_ctx.device, s.layout,          nullptr);
        vkDestroyPipelineLayout     (g_ctx.device, s.pipeline_layout, nullptr);
    });
    s_shaders.for_each([](ShaderSlot& s) {
        vkDestroyShaderModule(g_ctx.device, s.module, nullptr);
    });
    s_samplers.for_each([](SamplerSlot& s) {
        vkDestroySampler(g_ctx.device, s.sampler, nullptr);
    });
    s_textures.for_each([](TextureSlot& s) {
        if (s.is_swapchain) return;
        vkDestroyImageView(g_ctx.device, s.view, nullptr);
        vmaDestroyImage(g_ctx.allocator, s.image, s.allocation);
    });
    s_buffers.for_each([](BufferSlot& s) {
        vmaDestroyBuffer(g_ctx.allocator, s.buffer, s.allocation);
    });
    s_fences.for_each([](FenceSlot& s) {
        vkDestroyFence(g_ctx.device, s.fence, nullptr);
    });
    s_semaphores.for_each([](SemaphoreSlot& s) {
        vkDestroySemaphore(g_ctx.device, s.semaphore, nullptr);
    });

    vkDestroyDescriptorPool(g_ctx.device, g_ctx.desc_pool, nullptr);
    vkDestroyPipelineLayout(g_ctx.device, g_ctx.default_pipeline_layout, nullptr);
    vkDestroyCommandPool(g_ctx.device, g_ctx.gfx_pool,  nullptr);
    vkDestroyCommandPool(g_ctx.device, g_ctx.comp_pool, nullptr);
    vkDestroyCommandPool(g_ctx.device, g_ctx.xfr_pool,  nullptr);
    vmaDestroyAllocator(g_ctx.allocator);
    vkDestroyDevice(g_ctx.device, nullptr);
    if (g_ctx.debug_messenger)
    {
        auto pfn = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(g_ctx.instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (pfn) pfn(g_ctx.instance, g_ctx.debug_messenger, nullptr);
    }
    vkDestroyInstance(g_ctx.instance, nullptr);
    g_ctx = {};
}

// =============================================================
//  DEVICE INFO
// =============================================================

static void vk_get_device_info(rhi::DeviceInfo& out)
{
    strncpy(out.name, g_ctx.props.deviceName, sizeof(out.name) - 1);
    VkPhysicalDeviceMemoryProperties mem{};
    vkGetPhysicalDeviceMemoryProperties(g_ctx.phys_dev, &mem);
    out.vram_bytes = 0;
    for (u32 i = 0; i < mem.memoryHeapCount; ++i)
        if (mem.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            out.vram_bytes = std::max(out.vram_bytes, mem.memoryHeaps[i].size);
    out.max_push_constant_size = g_ctx.props.limits.maxPushConstantsSize;
    out.max_anisotropy         = static_cast<u32>(g_ctx.props.limits.maxSamplerAnisotropy);
}

// =============================================================
//  BUFFERS
// =============================================================

static rhi::Buffer vk_buffer_create(const rhi::BufferDesc& desc)
{
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                             | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    if (has(desc.usage, rhi::BufferUsage::Vertex))   usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (has(desc.usage, rhi::BufferUsage::Index))    usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (has(desc.usage, rhi::BufferUsage::Uniform))  usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (has(desc.usage, rhi::BufferUsage::Storage))  usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (has(desc.usage, rhi::BufferUsage::Indirect)) usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

    const VkBufferCreateInfo bci{
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = desc.size,
        .usage       = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    const VmaAllocationCreateInfo aci{
        .flags = (desc.memory != rhi::MemoryType::GpuOnly)
                    ? VMA_ALLOCATION_CREATE_MAPPED_BIT : (VmaAllocationCreateFlags)0,
        .usage = to_vma_usage(desc.memory),
    };

    rhi::Buffer h = s_buffers.alloc();
    if (!h) return {};
    BufferSlot& slot = s_buffers.get_checked(h);
    slot.size  = desc.size;
    slot.usage = desc.usage;

    VmaAllocationInfo alloc_info{};
    if (vmaCreateBuffer(g_ctx.allocator, &bci, &aci,
                        &slot.buffer, &slot.allocation, &alloc_info) != VK_SUCCESS)
    {
        fprintf(stderr, "[vkrhi] vmaCreateBuffer failed (size=%llu)\n", (unsigned long long)desc.size);
        s_buffers.free(h);
        return {};
    }

    slot.mapped_ptr = alloc_info.pMappedData; // non-null if MAPPED_BIT was set
    slot.persistent_map = (slot.mapped_ptr != nullptr);

    // Buffer device address (useful for bindless / ray tracing)
    const VkBufferDeviceAddressInfo bda{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = slot.buffer };
    slot.gpu_address = vkGetBufferDeviceAddress(g_ctx.device, &bda);

    set_debug_name(VK_OBJECT_TYPE_BUFFER, reinterpret_cast<u64>(slot.buffer), desc.name);
    return h;
}

static void vk_buffer_destroy(rhi::Buffer h)
{
    BufferSlot* s = s_buffers.get(h);
    if (!s) return;
    vmaDestroyBuffer(g_ctx.allocator, s->buffer, s->allocation);
    s_buffers.free(h);
}

static rhi::MappedBuffer vk_buffer_map(rhi::Buffer h)
{
    BufferSlot* s = s_buffers.get(h);
    if (!s) return {};
    if (!s->mapped_ptr)
        vmaMapMemory(g_ctx.allocator, s->allocation, &s->mapped_ptr);
    return { s->mapped_ptr, s->size, h };
}

static void vk_buffer_unmap(rhi::Buffer h)
{
    BufferSlot* s = s_buffers.get(h);
    if (!s || !s->mapped_ptr) return;
    
    if (!s->persistent_map)
    {
        vmaUnmapMemory(g_ctx.allocator, s->allocation);
        // Reset the mapped pointer in the slot to nullptr.
        s->mapped_ptr = nullptr;
    }
}

static void vk_buffer_flush(rhi::Buffer h, u64 offset, u64 size)
{
    BufferSlot* s = s_buffers.get(h);
    if (!s) return;
    vmaFlushAllocation(g_ctx.allocator, s->allocation, offset, size ? size : VK_WHOLE_SIZE);
}

// =============================================================
//  TEXTURES
// =============================================================

static rhi::Texture vk_texture_create(const rhi::TextureDesc& desc)
{
    const VkFormat fmt = to_vk_format(desc.format);

    VkImageUsageFlags usage = 0;
    if (has(desc.usage, rhi::TextureUsage::Sampled))     usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (has(desc.usage, rhi::TextureUsage::Storage))     usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    if (has(desc.usage, rhi::TextureUsage::ColorTarget)) usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (has(desc.usage, rhi::TextureUsage::DepthTarget)) usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (has(desc.usage, rhi::TextureUsage::TransferSrc)) usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (has(desc.usage, rhi::TextureUsage::TransferDst)) usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    const VkImageType img_type =
        desc.dim == rhi::TextureDim::D1 ? VK_IMAGE_TYPE_1D :
        desc.dim == rhi::TextureDim::D3 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;

    const bool is_cube = (desc.dim == rhi::TextureDim::Cube || desc.dim == rhi::TextureDim::CubeArray);

    const VkImageCreateInfo ici{
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags         = is_cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : (VkImageCreateFlags)0,
        .imageType     = img_type,
        .format        = fmt,
        .extent        = { desc.width, desc.height, desc.depth },
        .mipLevels     = desc.mip_levels,
        .arrayLayers   = desc.array_layers,
        .samples       = static_cast<VkSampleCountFlagBits>(desc.sample_count),
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = usage,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    const VmaAllocationCreateInfo aci{ .usage = VMA_MEMORY_USAGE_GPU_ONLY };

    rhi::Texture h = s_textures.alloc();
    if (!h) return {};
    TextureSlot& slot = s_textures.get_checked(h);
    slot.format       = fmt;
    slot.width        = desc.width;
    slot.height       = desc.height;
    slot.depth        = desc.depth;
    slot.mip_levels   = desc.mip_levels;
    slot.array_layers = desc.array_layers;
    slot.sample_count = desc.sample_count;

    if (vmaCreateImage(g_ctx.allocator, &ici, &aci,
                       &slot.image, &slot.allocation, nullptr) != VK_SUCCESS)
    {
        fprintf(stderr, "[vkrhi] vmaCreateImage failed (%ux%u fmt=%u)\n",
                desc.width, desc.height, static_cast<u32>(desc.format));
        s_textures.free(h);
        return {};
    }

    // Default image view (all mips, all layers)
    const VkImageViewType vt =
        desc.dim == rhi::TextureDim::D1         ? VK_IMAGE_VIEW_TYPE_1D :
        desc.dim == rhi::TextureDim::D2         ? VK_IMAGE_VIEW_TYPE_2D :
        desc.dim == rhi::TextureDim::D3         ? VK_IMAGE_VIEW_TYPE_3D :
        desc.dim == rhi::TextureDim::Cube       ? VK_IMAGE_VIEW_TYPE_CUBE :
        desc.dim == rhi::TextureDim::D2Array    ? VK_IMAGE_VIEW_TYPE_2D_ARRAY :
                                                  VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;

    const VkImageViewCreateInfo vci{
        .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image      = slot.image,
        .viewType   = vt,
        .format     = fmt,
        .subresourceRange = {
            .aspectMask     = aspect_for_format(fmt),
            .baseMipLevel   = 0,
            .levelCount     = desc.mip_levels,
            .baseArrayLayer = 0,
            .layerCount     = desc.array_layers,
        },
    };
    vkCreateImageView(g_ctx.device, &vci, nullptr, &slot.view);

    set_debug_name(VK_OBJECT_TYPE_IMAGE,      reinterpret_cast<u64>(slot.image), desc.name);
    set_debug_name(VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<u64>(slot.view),  desc.name);
    return h;
}

static void vk_texture_destroy(rhi::Texture h)
{
    TextureSlot* s = s_textures.get(h);
    if (!s) return;
    if (s->imgui_ds)
    {
        ImGui_ImplVulkan_RemoveTexture(s->imgui_ds);
    }
    if (!s->is_swapchain)
    {
        vkDestroyImageView(g_ctx.device, s->view, nullptr);
        vmaDestroyImage(g_ctx.allocator, s->image, s->allocation);
    }
    s_textures.free(h);
}

// =============================================================
//  SAMPLERS
// =============================================================

static rhi::Sampler vk_sampler_create(const rhi::SamplerDesc& desc)
{
    const VkSamplerCreateInfo sci{
        .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter               = to_vk_filter(desc.mag_filter),
        .minFilter               = to_vk_filter(desc.min_filter),
        .mipmapMode              = to_vk_mipmap(desc.mip_mode),
        .addressModeU            = to_vk_address(desc.address_u),
        .addressModeV            = to_vk_address(desc.address_v),
        .addressModeW            = to_vk_address(desc.address_w),
        .mipLodBias              = desc.mip_bias,
        .anisotropyEnable        = desc.anisotropy ? VK_TRUE : VK_FALSE,
        .maxAnisotropy           = desc.max_aniso,
        .compareEnable           = desc.compare ? VK_TRUE : VK_FALSE,
        .compareOp               = to_vk_compare_op(desc.compare_op),
        .minLod                  = desc.min_lod,
        .maxLod                  = desc.max_lod,
        .unnormalizedCoordinates = desc.unnorm_coords ? VK_TRUE : VK_FALSE,
    };

    rhi::Sampler h = s_samplers.alloc();
    if (!h) return {};
    SamplerSlot& slot = s_samplers.get_checked(h);
    vkCreateSampler(g_ctx.device, &sci, nullptr, &slot.sampler);
    set_debug_name(VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<u64>(slot.sampler), desc.name);
    return h;
}

static void vk_sampler_destroy(rhi::Sampler h)
{
    SamplerSlot* s = s_samplers.get(h);
    if (!s) return;
    vkDestroySampler(g_ctx.device, s->sampler, nullptr);
    s_samplers.free(h);
}

// =============================================================
//  SHADERS
// =============================================================

static rhi::Shader vk_shader_create(const rhi::ShaderDesc& desc)
{
    rhi::Shader h = s_shaders.alloc();
    if (!h) return {};
    ShaderSlot& slot = s_shaders.get_checked(h);
    slot.stage = desc.stage;
    strncpy(slot.entry, desc.entry ? desc.entry : "main", sizeof(slot.entry) - 1);

    const VkShaderModuleCreateInfo mci{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = desc.size,
        .pCode    = static_cast<const u32*>(desc.bytecode),
    };
    if (vkCreateShaderModule(g_ctx.device, &mci, nullptr, &slot.module) != VK_SUCCESS)
    {
        fprintf(stderr, "[vkrhi] vkCreateShaderModule failed\n");
        s_shaders.free(h);
        return {};
    }
    set_debug_name(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<u64>(slot.module), desc.name);
    return h;
}

static void vk_shader_destroy(rhi::Shader h)
{
    ShaderSlot* s = s_shaders.get(h);
    if (!s) return;
    vkDestroyShaderModule(g_ctx.device, s->module, nullptr);
    s_shaders.free(h);
}

// =============================================================
//  DESCRIPTOR LAYOUTS
// =============================================================

static rhi::DescriptorLayout vk_descriptor_layout_create(const rhi::DescriptorLayoutDesc& desc)
{
    VkDescriptorSetLayoutBinding bindings[rhi::MAX_DESCRIPTOR_BINDINGS];
    for (u32 i = 0; i < desc.count; ++i)
    {
        bindings[i] = {
            .binding         = desc.bindings[i].binding,
            .descriptorType  = to_vk_desc_type(desc.bindings[i].type),
            .descriptorCount = desc.bindings[i].count,
            .stageFlags      = to_vk_shader_stage(desc.bindings[i].stages),
        };
    }
    const VkDescriptorSetLayoutCreateInfo dslci{
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = desc.count,
        .pBindings    = bindings,
    };

    rhi::DescriptorLayout h = s_desc_layouts.alloc();
    if (!h) return {};
    DescLayoutSlot& slot = s_desc_layouts.get_checked(h);
    vkCreateDescriptorSetLayout(g_ctx.device, &dslci, nullptr, &slot.layout);

    const VkPushConstantRange pcr{
        .stageFlags = VK_SHADER_STAGE_ALL,
        .offset     = 0,
        .size       = 256,
    };
    const VkPipelineLayoutCreateInfo plci{
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = 1,
        .pSetLayouts            = &slot.layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &pcr,
    };
    vkCreatePipelineLayout(g_ctx.device, &plci, nullptr, &slot.pipeline_layout);

    set_debug_name(VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                   reinterpret_cast<u64>(slot.layout), desc.name);
    return h;
}

static void vk_descriptor_layout_destroy(rhi::DescriptorLayout h)
{
    DescLayoutSlot* s = s_desc_layouts.get(h);
    if (!s) return;
    vkDestroyDescriptorSetLayout(g_ctx.device, s->layout,          nullptr);
    vkDestroyPipelineLayout     (g_ctx.device, s->pipeline_layout, nullptr);
    s_desc_layouts.free(h);
}

// =============================================================
//  DESCRIPTOR SETS
// =============================================================

static rhi::DescriptorSet vk_descriptor_set_create(rhi::DescriptorLayout layout_h)
{
    DescLayoutSlot* layout = s_desc_layouts.get(layout_h);
    if (!layout) return {};

    const VkDescriptorSetAllocateInfo ai{
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = g_ctx.desc_pool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &layout->layout,
    };
    rhi::DescriptorSet h = s_desc_sets.alloc();
    if (!h) return {};
    DescSetSlot& slot = s_desc_sets.get_checked(h);
    slot.layout_handle = layout_h;
    if (vkAllocateDescriptorSets(g_ctx.device, &ai, &slot.set) != VK_SUCCESS)
    {
        fprintf(stderr, "[vkrhi] vkAllocateDescriptorSets failed — descriptor pool full?\n");
        s_desc_sets.free(h);
        return {};
    }
    return h;
}

static void vk_descriptor_set_destroy(rhi::DescriptorSet h)
{
    DescSetSlot* s = s_desc_sets.get(h);
    if (!s) return;
    vkFreeDescriptorSets(g_ctx.device, g_ctx.desc_pool, 1, &s->set);
    s_desc_sets.free(h);
}

static void vk_descriptor_set_write(rhi::DescriptorSet dsh,
                                     const rhi::DescriptorWrite* writes, u32 count)
{
    DescSetSlot* ds = s_desc_sets.get(dsh);
    if (!ds) return;

    VkWriteDescriptorSet  vk_writes[rhi::MAX_DESCRIPTOR_BINDINGS];
    VkDescriptorBufferInfo buf_infos[rhi::MAX_DESCRIPTOR_BINDINGS];
    VkDescriptorImageInfo  img_infos[rhi::MAX_DESCRIPTOR_BINDINGS];

    count = std::min(count, rhi::MAX_DESCRIPTOR_BINDINGS);
    for (u32 i = 0; i < count; ++i)
    {
        const auto& w = writes[i];
        vk_writes[i] = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = ds->set,
            .dstBinding      = w.binding,
            .dstArrayElement = w.array_index,
            .descriptorCount = 1,
            .descriptorType  = to_vk_desc_type(w.type),
        };
        switch (w.type)
        {
            case rhi::DescriptorType::UniformBuffer:
            case rhi::DescriptorType::UniformBufferDynamic:
            case rhi::DescriptorType::StorageBuffer:
            case rhi::DescriptorType::StorageBufferDynamic:
            {
                BufferSlot* bs = s_buffers.get(w.buffer.buf);
                buf_infos[i] = {
                    .buffer = bs ? bs->buffer : VK_NULL_HANDLE,
                    .offset = w.buffer.offset,
                    .range  = w.buffer.range == ~0ull ? VK_WHOLE_SIZE : w.buffer.range,
                };
                vk_writes[i].pBufferInfo = &buf_infos[i];
                break;
            }
            case rhi::DescriptorType::CombinedImageSampler:
            case rhi::DescriptorType::SampledTexture:
            case rhi::DescriptorType::StorageTexture:
            case rhi::DescriptorType::InputAttachment:
            {
                TextureSlot* ts  = s_textures.get(w.texture.tex);
                SamplerSlot* ss  = s_samplers.get(w.texture.samp);
                img_infos[i] = {
                    .sampler     = ss  ? ss->sampler       : VK_NULL_HANDLE,
                    .imageView   = ts  ? ts->view          : VK_NULL_HANDLE,
                    .imageLayout = to_vk_layout(w.texture.layout),
                };
                vk_writes[i].pImageInfo = &img_infos[i];
                break;
            }
            case rhi::DescriptorType::Sampler:
            {
                SamplerSlot* ss = s_samplers.get(w.texture.samp);
                img_infos[i] = { .sampler = ss ? ss->sampler : VK_NULL_HANDLE };
                vk_writes[i].pImageInfo = &img_infos[i];
                break;
            }
        }
    }
    vkUpdateDescriptorSets(g_ctx.device, count, vk_writes, 0, nullptr);
}

// =============================================================
//  PIPELINES
// =============================================================

static rhi::Pipeline vk_pipeline_create(const rhi::PipelineDesc& desc)
{
    // Compute pipeline
    if (desc.compute_shader)
    {
        ShaderSlot* cs = s_shaders.get(desc.compute_shader);
        if (!cs) return {};
        DescLayoutSlot* layout = s_desc_layouts.get(desc.layout);
        VkPipelineLayout vk_layout = layout ? layout->pipeline_layout : g_ctx.default_pipeline_layout;

        const VkPipelineShaderStageCreateInfo stage{
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = cs->module,
            .pName  = cs->entry,
        };
        const VkComputePipelineCreateInfo ci{
            .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage  = stage,
            .layout = vk_layout,
        };
        rhi::Pipeline h = s_pipelines.alloc();
        if (!h) return {};
        PipelineSlot& slot = s_pipelines.get_checked(h);
        slot.bind_point     = VK_PIPELINE_BIND_POINT_COMPUTE;
        slot.pipeline_layout= layout ? layout->pipeline_layout : VK_NULL_HANDLE;
        slot.pipeline_layout= vk_layout;
        vkCreateComputePipelines(g_ctx.device, VK_NULL_HANDLE, 1, &ci, nullptr, &slot.pipeline);
        set_debug_name(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<u64>(slot.pipeline), desc.name);
        return h;
    }

    // Graphics pipeline
    ShaderSlot* vs = s_shaders.get(desc.vertex_shader);
    ShaderSlot* fs = s_shaders.get(desc.fragment_shader);
    DescLayoutSlot* layout = s_desc_layouts.get(desc.layout);
    VkPipelineLayout vk_layout = layout ? layout->pipeline_layout : g_ctx.default_pipeline_layout;

    // Shader stages
    VkPipelineShaderStageCreateInfo stages[2];
    u32 stage_count = 0;
    if (vs) stages[stage_count++] = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vs->module, .pName = vs->entry,
    };
    if (fs) stages[stage_count++] = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fs->module, .pName = fs->entry,
    };

    // Vertex input
    VkVertexInputBindingDescription   vk_bindings[rhi::MAX_VERTEX_BUFFERS];
    VkVertexInputAttributeDescription vk_attribs [rhi::MAX_VERTEX_ATTRIBS];
    for (u32 i = 0; i < desc.binding_count; ++i)
        vk_bindings[i] = {
            .binding   = desc.bindings[i].binding,
            .stride    = desc.bindings[i].stride,
            .inputRate = desc.bindings[i].input_rate == rhi::VertexInputRate::Instance
                         ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX,
        };
    for (u32 i = 0; i < desc.attrib_count; ++i)
        vk_attribs[i] = {
            .location = desc.attribs[i].location,
            .binding  = desc.attribs[i].binding,
            .format   = to_vk_format(desc.attribs[i].format),
            .offset   = desc.attribs[i].offset,
        };
    const VkPipelineVertexInputStateCreateInfo vis{
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = desc.binding_count,
        .pVertexBindingDescriptions      = vk_bindings,
        .vertexAttributeDescriptionCount = desc.attrib_count,
        .pVertexAttributeDescriptions    = vk_attribs,
    };

    // Input assembly
    const VkPipelineInputAssemblyStateCreateInfo ias{
        .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = to_vk_topology(desc.topology),
    };

    // Rasterizer
    const VkPipelineRasterizationStateCreateInfo ras{
        .sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = to_vk_fill(desc.raster.fill_mode),
        .cullMode    = static_cast<VkCullModeFlags>(to_vk_cull(desc.raster.cull_mode)),
        .frontFace   = to_vk_front_face(desc.raster.front_face),
        .lineWidth   = 1.f,
    };

    // Multisample
    const VkPipelineMultisampleStateCreateInfo mss{
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = static_cast<VkSampleCountFlagBits>(
                                    desc.sample_count ? desc.sample_count : 1),
        .alphaToCoverageEnable= desc.raster.alpha_to_coverage ? VK_TRUE : VK_FALSE,
    };

    // Depth stencil
    const VkPipelineDepthStencilStateCreateInfo ds{
        .sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable  = desc.depth.test_enable  ? VK_TRUE : VK_FALSE,
        .depthWriteEnable = desc.depth.write_enable ? VK_TRUE : VK_FALSE,
        .depthCompareOp   = to_vk_compare_op(desc.depth.compare_op),
        .minDepthBounds   = desc.depth.min_bounds,
        .maxDepthBounds   = desc.depth.max_bounds,
    };

    // Blend
    VkPipelineColorBlendAttachmentState blend_atts[rhi::MAX_COLOR_TARGETS];
    for (u32 i = 0; i < desc.color_count; ++i)
    {
        const auto& b = desc.blend[i];
        blend_atts[i] = {
            .blendEnable         = b.enable ? VK_TRUE : VK_FALSE,
            .srcColorBlendFactor = to_vk_blend_factor(b.src_color),
            .dstColorBlendFactor = to_vk_blend_factor(b.dst_color),
            .colorBlendOp        = to_vk_blend_op(b.color_op),
            .srcAlphaBlendFactor = to_vk_blend_factor(b.src_alpha),
            .dstAlphaBlendFactor = to_vk_blend_factor(b.dst_alpha),
            .alphaBlendOp        = to_vk_blend_op(b.alpha_op),
            .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                 | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
    }
    const VkPipelineColorBlendStateCreateInfo cbs{
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = desc.color_count,
        .pAttachments    = blend_atts,
    };

    // Viewport (dynamic — set at command time)
    static constexpr VkDynamicState k_dyn[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    const VkPipelineDynamicStateCreateInfo dyn{
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates    = k_dyn,
    };
    const VkPipelineViewportStateCreateInfo vps{
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1, .scissorCount = 1,
    };

    // Render target formats for dynamic rendering (no VkRenderPass needed)
    VkFormat color_fmts[rhi::MAX_COLOR_TARGETS];
    for (u32 i = 0; i < desc.color_count; ++i)
        color_fmts[i] = to_vk_format(desc.color_formats[i]);

    const VkPipelineRenderingCreateInfo prc{
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount    = desc.color_count,
        .pColorAttachmentFormats = color_fmts,
        .depthAttachmentFormat   = desc.depth_format != rhi::Format::Undefined
                                       ? to_vk_format(desc.depth_format) : VK_FORMAT_UNDEFINED,
        .stencilAttachmentFormat = desc.stencil_format != rhi::Format::Undefined
                                       ? to_vk_format(desc.stencil_format) : VK_FORMAT_UNDEFINED,
    };

    const VkGraphicsPipelineCreateInfo gci{
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &prc,
        .stageCount          = stage_count,
        .pStages             = stages,
        .pVertexInputState   = &vis,
        .pInputAssemblyState = &ias,
        .pViewportState      = &vps,
        .pRasterizationState = &ras,
        .pMultisampleState   = &mss,
        .pDepthStencilState  = &ds,
        .pColorBlendState    = &cbs,
        .pDynamicState       = &dyn,
        .layout              = vk_layout
    };

    rhi::Pipeline h = s_pipelines.alloc();
    if (!h) return {};
    PipelineSlot& slot = s_pipelines.get_checked(h);
    slot.bind_point      = VK_PIPELINE_BIND_POINT_GRAPHICS;
    slot.pipeline_layout = layout ? layout->pipeline_layout : VK_NULL_HANDLE;
    slot.pipeline_layout = vk_layout;

    if (vkCreateGraphicsPipelines(g_ctx.device, VK_NULL_HANDLE, 1, &gci, nullptr, &slot.pipeline) != VK_SUCCESS)
    {
        fprintf(stderr, "[vkrhi] vkCreateGraphicsPipelines failed (%s)\n", desc.name ? desc.name : "?");
        s_pipelines.free(h);
        return {};
    }
    set_debug_name(VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<u64>(slot.pipeline), desc.name);
    return h;
}

static void vk_pipeline_destroy(rhi::Pipeline h)
{
    PipelineSlot* s = s_pipelines.get(h);
    if (!s) return;
    vkDestroyPipeline(g_ctx.device, s->pipeline, nullptr);
    s_pipelines.free(h);
}

// =============================================================
//  COMMAND BUFFERS
// =============================================================

static rhi::CmdBuf vk_cmdbuf_create(u32 qf)
{
    VkCommandPool pool = qf == 0 ? g_ctx.gfx_pool : qf == 1 ? g_ctx.comp_pool : g_ctx.xfr_pool;
    const VkCommandBufferAllocateInfo ai{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    rhi::CmdBuf h = s_cmdbufs.alloc();
    if (!h) return {};
    CmdBufSlot& slot = s_cmdbufs.get_checked(h);
    slot.queue_family = qf;
    if (vkAllocateCommandBuffers(g_ctx.device, &ai, &slot.cmd) != VK_SUCCESS)
    { s_cmdbufs.free(h); return {}; }
    return h;
}

static void vk_cmdbuf_destroy(rhi::CmdBuf h)
{
    CmdBufSlot* s = s_cmdbufs.get(h);
    if (!s) return;
    VkCommandPool pool = s->queue_family == 0 ? g_ctx.gfx_pool
                       : s->queue_family == 1 ? g_ctx.comp_pool : g_ctx.xfr_pool;
    vkFreeCommandBuffers(g_ctx.device, pool, 1, &s->cmd);
    s_cmdbufs.free(h);
}

static void vk_cmdbuf_begin(rhi::CmdBuf h)
{
    CmdBufSlot* s = s_cmdbufs.get(h);
    if (!s) return;
    const VkCommandBufferBeginInfo bi{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(s->cmd, &bi);
    s->recording = true;
}

static void vk_cmdbuf_end(rhi::CmdBuf h)
{
    if (CmdBufSlot* s = s_cmdbufs.get(h)) { vkEndCommandBuffer(s->cmd); s->recording = false; }
}

static void vk_cmdbuf_reset(rhi::CmdBuf h)
{
    if (CmdBufSlot* s = s_cmdbufs.get(h)) { vkResetCommandBuffer(s->cmd, 0); s->recording = false; }
}

// =============================================================
//  RENDER PASS (VK 1.3 dynamic rendering)
// =============================================================

static void vk_cmd_begin_render_pass(rhi::CmdBuf ch, const rhi::RenderPassDesc& rp)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    if (!cmd) return;

    VkRenderingAttachmentInfo color_atts[rhi::MAX_COLOR_TARGETS] = {};
    for (u32 i = 0; i < rp.color_count; ++i)
    {
        const auto& a   = rp.color[i];
        TextureSlot* tx = s_textures.get(a.texture);
        TextureSlot* rs = s_textures.get(a.resolve_texture);
        color_atts[i] = {
            .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView          = tx ? tx->view : VK_NULL_HANDLE,
            .imageLayout        = to_vk_layout(a.layout),
            .resolveMode        = rs ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE,
            .resolveImageView   = rs ? rs->view : VK_NULL_HANDLE,
            .resolveImageLayout = rs ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp  = a.load_op  == rhi::LoadOp::Clear    ? VK_ATTACHMENT_LOAD_OP_CLEAR
                     : a.load_op  == rhi::LoadOp::Load     ? VK_ATTACHMENT_LOAD_OP_LOAD
                                                           : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = a.store_op == rhi::StoreOp::Store   ? VK_ATTACHMENT_STORE_OP_STORE
                                                           : VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue = { .color = { .float32 = { a.clear.r, a.clear.g, a.clear.b, a.clear.a } } },
        };
    }

    VkRenderingAttachmentInfo depth_att{};
    if (rp.has_depth)
    {
        TextureSlot* tx = s_textures.get(rp.depth.texture);
        depth_att = {
            .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView   = tx ? tx->view : VK_NULL_HANDLE,
            .imageLayout = to_vk_layout(rp.depth.layout),
            .loadOp  = rp.depth.load_op  == rhi::LoadOp::Clear ? VK_ATTACHMENT_LOAD_OP_CLEAR
                     : rp.depth.load_op  == rhi::LoadOp::Load  ? VK_ATTACHMENT_LOAD_OP_LOAD
                                                                : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = rp.depth.store_op == rhi::StoreOp::Store ? VK_ATTACHMENT_STORE_OP_STORE
                                                                 : VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue = { .depthStencil = { rp.depth.clear.depth, rp.depth.clear.stencil } },
        };
    }

    // Extent is derived from the first colour attachment (or depth if no colour)
    u32 w = 0, h = 0;
    if (rp.color_count > 0) { auto* tx = s_textures.get(rp.color[0].texture); if (tx) { w = tx->width; h = tx->height; } }
    else if (rp.has_depth) { auto* tx = s_textures.get(rp.depth.texture); if (tx) { w = tx->width; h = tx->height; } }

    const VkRenderingInfo ri{
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea           = { {0,0}, {w, h} },
        .layerCount           = 1,
        .colorAttachmentCount = rp.color_count,
        .pColorAttachments    = color_atts,
        .pDepthAttachment     = rp.has_depth ? &depth_att : nullptr,
    };
    vkCmdBeginRendering(cmd->cmd, &ri);
}

static void vk_cmd_end_render_pass(rhi::CmdBuf ch)
{
    if (CmdBufSlot* s = s_cmdbufs.get(ch)) vkCmdEndRendering(s->cmd);
}

// =============================================================
//  STATE BINDING
// =============================================================

static void vk_cmd_set_viewport(rhi::CmdBuf ch, const rhi::Viewport& vp)
{
    if (CmdBufSlot* s = s_cmdbufs.get(ch))
    {
        // Flip Y for Vulkan clip space (NDC Y points down in VK)
        const VkViewport v{ vp.x, vp.y + vp.h, vp.w, -vp.h, vp.min_depth, vp.max_depth };
        vkCmdSetViewport(s->cmd, 0, 1, &v);
    }
}

static void vk_cmd_set_scissor(rhi::CmdBuf ch, const rhi::Rect& r)
{
    if (CmdBufSlot* s = s_cmdbufs.get(ch))
    {
        const VkRect2D rect{ { r.x, r.y }, { r.w, r.h } };
        vkCmdSetScissor(s->cmd, 0, 1, &rect);
    }
}

static void vk_cmd_bind_pipeline(rhi::CmdBuf ch, rhi::Pipeline ph)
{
    CmdBufSlot*  cmd  = s_cmdbufs.get(ch);
    PipelineSlot* pip = s_pipelines.get(ph);
    if (!cmd || !pip) return;
    vkCmdBindPipeline(cmd->cmd, pip->bind_point, pip->pipeline);
    cmd->bound_pipeline_layout = pip->pipeline_layout;
}

static void vk_cmd_bind_descriptor_set(rhi::CmdBuf ch, rhi::DescriptorSet dsh,
                                        u32 set_index,
                                        const u32* dyn_offsets, u32 dyn_count)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    DescSetSlot* ds = s_desc_sets.get(dsh);
    if (!cmd || !ds) return;

    // Look up pipeline layout from the descriptor set's layout slot
    DescLayoutSlot* layout = s_desc_layouts.get(ds->layout_handle);
    if (!layout) return;

    vkCmdBindDescriptorSets(cmd->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                             layout->pipeline_layout, set_index,
                             1, &ds->set,
                             dyn_count, dyn_offsets);
}

static void vk_cmd_bind_vertex_buffers(rhi::CmdBuf ch, u32 first,
                                        const rhi::Buffer* bufs,
                                        const u64* offsets, u32 count)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    if (!cmd) return;
    VkBuffer     vk_bufs[rhi::MAX_VERTEX_BUFFERS];
    VkDeviceSize vk_offs[rhi::MAX_VERTEX_BUFFERS];
    count = std::min(count, rhi::MAX_VERTEX_BUFFERS);
    for (u32 i = 0; i < count; ++i)
    {
        BufferSlot* b = s_buffers.get(bufs[i]);
        vk_bufs[i] = b ? b->buffer : VK_NULL_HANDLE;
        vk_offs[i] = offsets ? offsets[i] : 0;
    }
    vkCmdBindVertexBuffers(cmd->cmd, first, count, vk_bufs, vk_offs);
}

static void vk_cmd_bind_index_buffer(rhi::CmdBuf ch, rhi::Buffer bh, u64 off, rhi::IndexType t)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    BufferSlot* buf = s_buffers.get(bh);
    if (!cmd || !buf) return;
    vkCmdBindIndexBuffer(cmd->cmd, buf->buffer, off,
        t == rhi::IndexType::Uint32 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
}

static void vk_cmd_push_constants(rhi::CmdBuf ch, rhi::ShaderStage stages,
                                   u32 offset, u32 size, const void* data)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    if (!cmd) return;
    vkCmdPushConstants(cmd->cmd, cmd->bound_pipeline_layout,
                       VK_SHADER_STAGE_ALL, offset, size, data);
}

// =============================================================
//  DRAW / DISPATCH
// =============================================================

static void vk_cmd_draw(rhi::CmdBuf ch, const rhi::DrawCmd& d)
{
    if (CmdBufSlot* s = s_cmdbufs.get(ch))
        vkCmdDraw(s->cmd, d.vertex_count, d.instance_count, d.first_vertex, d.first_instance);
}

static void vk_cmd_draw_indexed(rhi::CmdBuf ch, const rhi::DrawIndexedCmd& d)
{
    if (CmdBufSlot* s = s_cmdbufs.get(ch))  
        vkCmdDrawIndexed(s->cmd, d.index_count, d.instance_count,
                         d.first_index, d.vertex_offset, d.first_instance);
}

static void vk_cmd_draw_indirect(rhi::CmdBuf ch, rhi::Buffer ah, u64 off, u32 dc, u32 stride)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    BufferSlot* args= s_buffers.get(ah);
    if (!cmd || !args) return;
    vkCmdDrawIndirect(cmd->cmd, args->buffer, off, dc, stride);
}

static void vk_cmd_draw_indexed_indirect(rhi::CmdBuf ch, rhi::Buffer ah, u64 off, u32 dc, u32 stride)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    BufferSlot* args= s_buffers.get(ah);
    if (!cmd || !args) return;
    vkCmdDrawIndexedIndirect(cmd->cmd, args->buffer, off, dc, stride);
}

static void vk_cmd_dispatch(rhi::CmdBuf ch, const rhi::DispatchCmd& d)
{
    if (CmdBufSlot* s = s_cmdbufs.get(ch))
        vkCmdDispatch(s->cmd, d.x, d.y, d.z);
}

static void vk_cmd_dispatch_indirect(rhi::CmdBuf ch, rhi::Buffer ah, u64 off)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    BufferSlot* args= s_buffers.get(ah);
    if (!cmd || !args) return;
    vkCmdDispatchIndirect(cmd->cmd, args->buffer, off);
}

// =============================================================
//  BARRIERS  (VK 1.3 synchronization2)
// =============================================================

static void vk_cmd_texture_barrier(rhi::CmdBuf ch, const rhi::TextureBarrier* bs, u32 n)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    if (!cmd) return;

    VkImageMemoryBarrier2 vk_bs[16];
    n = std::min(n, 16u);
    for (u32 i = 0; i < n; ++i)
    {
        const auto& b = bs[i];
        TextureSlot* tx = s_textures.get(b.tex);
        vk_bs[i] = {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask        = to_vk_stage2(b.src_stage),
            .srcAccessMask       = to_vk_access2(b.src_access),
            .dstStageMask        = to_vk_stage2(b.dst_stage),
            .dstAccessMask       = to_vk_access2(b.dst_access),
            .oldLayout           = to_vk_layout(b.old_layout),
            .newLayout           = to_vk_layout(b.new_layout),
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = tx ? tx->image : VK_NULL_HANDLE,
            .subresourceRange    = {
                .aspectMask     = tx ? aspect_for_format(tx->format) : VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = b.base_mip,
                .levelCount     = b.mip_count   ? b.mip_count   : VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = b.base_layer,
                .layerCount     = b.layer_count ? b.layer_count : VK_REMAINING_ARRAY_LAYERS,
            },
        };
        // Update cached layout
        if (tx) tx->layout = to_vk_layout(b.new_layout);
    }
    const VkDependencyInfo dep{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                .imageMemoryBarrierCount = n, .pImageMemoryBarriers = vk_bs };
    vkCmdPipelineBarrier2(cmd->cmd, &dep);
}

static void vk_cmd_buffer_barrier(rhi::CmdBuf ch, const rhi::BufferBarrier* bs, u32 n)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    if (!cmd) return;

    VkBufferMemoryBarrier2 vk_bs[16];
    n = std::min(n, 16u);
    for (u32 i = 0; i < n; ++i)
    {
        const auto& b = bs[i];
        BufferSlot* buf = s_buffers.get(b.buf);
        vk_bs[i] = {
            .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask        = to_vk_stage2(b.src_stage),
            .srcAccessMask       = to_vk_access2(b.src_access),
            .dstStageMask        = to_vk_stage2(b.dst_stage),
            .dstAccessMask       = to_vk_access2(b.dst_access),
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer              = buf ? buf->buffer : VK_NULL_HANDLE,
            .offset              = b.offset,
            .size                = b.size ? b.size : VK_WHOLE_SIZE,
        };
    }
    const VkDependencyInfo dep{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                .bufferMemoryBarrierCount = n, .pBufferMemoryBarriers = vk_bs };
    vkCmdPipelineBarrier2(cmd->cmd, &dep);
}

static void vk_cmd_global_barrier(rhi::CmdBuf ch,
                                   rhi::PipelineStage src_stage, rhi::Access src_access,
                                   rhi::PipelineStage dst_stage, rhi::Access dst_access)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    if (!cmd) return;
    const VkMemoryBarrier2 mb{
        .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
        .srcStageMask  = to_vk_stage2(src_stage),
        .srcAccessMask = to_vk_access2(src_access),
        .dstStageMask  = to_vk_stage2(dst_stage),
        .dstAccessMask = to_vk_access2(dst_access),
    };
    const VkDependencyInfo dep{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                .memoryBarrierCount = 1, .pMemoryBarriers = &mb };
    vkCmdPipelineBarrier2(cmd->cmd, &dep);
}

// =============================================================
//  TRANSFER
// =============================================================

static void vk_cmd_copy_buffer(rhi::CmdBuf ch, const rhi::BufferCopy& c)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    BufferSlot* src = s_buffers.get(c.src);
    BufferSlot* dst = s_buffers.get(c.dst);
    if (!cmd || !src || !dst) return;
    const VkBufferCopy region{ c.src_offset, c.dst_offset, c.size };
    vkCmdCopyBuffer(cmd->cmd, src->buffer, dst->buffer, 1, &region);
}

static void vk_cmd_copy_buffer_to_texture(rhi::CmdBuf ch, const rhi::BufferTextureCopy& c)
{
    CmdBufSlot*  cmd = s_cmdbufs.get(ch);
    BufferSlot*  buf = s_buffers.get(c.buf);
    TextureSlot* tex = s_textures.get(c.tex);
    if (!cmd || !buf || !tex) return;

    const VkBufferImageCopy region{
        .bufferOffset      = c.buf_offset,
        .bufferRowLength   = c.buf_row_len,
        .bufferImageHeight = c.buf_img_h,
        .imageSubresource  = {
            .aspectMask     = aspect_for_format(tex->format),
            .mipLevel       = c.mip,
            .baseArrayLayer = c.base_layer,
            .layerCount     = c.layer_count,
        },
        .imageOffset = { static_cast<i32>(c.x), static_cast<i32>(c.y), static_cast<i32>(c.z) },
        .imageExtent = { c.w ? c.w : tex->width >> c.mip,
                         c.h ? c.h : tex->height >> c.mip,
                         c.d },
    };
    vkCmdCopyBufferToImage(cmd->cmd, buf->buffer, tex->image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

static void vk_cmd_copy_texture_to_buffer(rhi::CmdBuf ch, const rhi::BufferTextureCopy& c)
{
    CmdBufSlot*  cmd = s_cmdbufs.get(ch);
    BufferSlot*  buf = s_buffers.get(c.buf);
    TextureSlot* tex = s_textures.get(c.tex);
    if (!cmd || !buf || !tex) return;
    const VkBufferImageCopy region{
        .bufferOffset      = c.buf_offset,
        .imageSubresource  = { aspect_for_format(tex->format), c.mip, c.base_layer, c.layer_count },
        .imageOffset       = { (i32)c.x, (i32)c.y, (i32)c.z },
        .imageExtent       = { c.w ? c.w : tex->width, c.h ? c.h : tex->height, c.d },
    };
    vkCmdCopyImageToBuffer(cmd->cmd, tex->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           buf->buffer, 1, &region);
}

static void vk_cmd_blit_texture(rhi::CmdBuf ch, rhi::Texture srch, rhi::Texture dsth,
                                 const rhi::Rect& sr, const rhi::Rect& dr,
                                 u32 sm, u32 dm, rhi::SamplerFilter filter)
{
    CmdBufSlot*  cmd = s_cmdbufs.get(ch);
    TextureSlot* src = s_textures.get(srch);
    TextureSlot* dst = s_textures.get(dsth);
    if (!cmd || !src || !dst) return;
    const VkImageBlit blit{
        .srcSubresource = { aspect_for_format(src->format), sm, 0, 1 },
        .srcOffsets     = { {(i32)sr.x,(i32)sr.y,0},
                            {(i32)(sr.x+sr.w),(i32)(sr.y+sr.h),1} },
        .dstSubresource = { aspect_for_format(dst->format), dm, 0, 1 },
        .dstOffsets     = { {(i32)dr.x,(i32)dr.y,0},
                            {(i32)(dr.x+dr.w),(i32)(dr.y+dr.h),1} },
    };
    vkCmdBlitImage(cmd->cmd,
                   src->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit, to_vk_filter(filter));
}

static void vk_cmd_clear_color_texture(rhi::CmdBuf ch, rhi::Texture th, const rhi::ClearColor& c,
                                        u32 bm, u32 nm, u32 bl, u32 nl)
{
    CmdBufSlot*  cmd = s_cmdbufs.get(ch);
    TextureSlot* tex = s_textures.get(th);
    if (!cmd || !tex) return;
    const VkClearColorValue cv{ .float32 = { c.r, c.g, c.b, c.a } };
    const VkImageSubresourceRange range{ VK_IMAGE_ASPECT_COLOR_BIT, bm, nm ? nm : VK_REMAINING_MIP_LEVELS, bl, nl ? nl : VK_REMAINING_ARRAY_LAYERS };
    vkCmdClearColorImage(cmd->cmd, tex->image, VK_IMAGE_LAYOUT_GENERAL, &cv, 1, &range);
}

static void vk_cmd_clear_depth_texture(rhi::CmdBuf ch, rhi::Texture th, const rhi::ClearDepth& c)
{
    CmdBufSlot*  cmd = s_cmdbufs.get(ch);
    TextureSlot* tex = s_textures.get(th);
    if (!cmd || !tex) return;
    const VkClearDepthStencilValue cv{ c.depth, c.stencil };
    const VkImageSubresourceRange range{ aspect_for_format(tex->format), 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
    vkCmdClearDepthStencilImage(cmd->cmd, tex->image, VK_IMAGE_LAYOUT_GENERAL, &cv, 1, &range);
}

static void vk_cmd_fill_buffer(rhi::CmdBuf ch, rhi::Buffer bh, u64 off, u64 size, u32 val)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    BufferSlot* buf = s_buffers.get(bh);
    if (!cmd || !buf) return;
    vkCmdFillBuffer(cmd->cmd, buf->buffer, off, size ? size : VK_WHOLE_SIZE, val);
}

static void vk_cmd_copy_texture(rhi::CmdBuf ch, const rhi::TextureCopy& c)
{
    CmdBufSlot*  cmd = s_cmdbufs.get(ch);
    TextureSlot* src = s_textures.get(c.src);
    TextureSlot* dst = s_textures.get(c.dst);
    if (!cmd || !src || !dst) return;
    const VkImageCopy region{
        .srcSubresource = { aspect_for_format(src->format), c.src_mip, c.src_layer, 1 },
        .srcOffset      = { (i32)c.src_x, (i32)c.src_y, (i32)c.src_z },
        .dstSubresource = { aspect_for_format(dst->format), c.dst_mip, c.dst_layer, 1 },
        .dstOffset      = { (i32)c.dst_x, (i32)c.dst_y, (i32)c.dst_z },
        .extent         = { c.width, c.height, c.depth },
    };
    vkCmdCopyImage(cmd->cmd,
                   src->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dst->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &region);
}

// =============================================================
//  DEBUG
// =============================================================

static void vk_cmd_begin_region(rhi::CmdBuf ch, const char* n, f32 r, f32 g, f32 b)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    if (!cmd || !g_ctx.pfn_begin_label) return;
    const VkDebugUtilsLabelEXT lbl{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = n, .color = { r, g, b, 1.f },
    };
    g_ctx.pfn_begin_label(cmd->cmd, &lbl);
}

static void vk_cmd_end_region(rhi::CmdBuf ch)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    if (cmd && g_ctx.pfn_end_label) g_ctx.pfn_end_label(cmd->cmd);
}

static void vk_cmd_marker(rhi::CmdBuf ch, const char* n)
{
    CmdBufSlot* cmd = s_cmdbufs.get(ch);
    if (!cmd || !g_ctx.pfn_insert_label) return;
    const VkDebugUtilsLabelEXT lbl{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = n, .color = { 1,1,1,1 },
    };
    g_ctx.pfn_insert_label(cmd->cmd, &lbl);
}

// =============================================================
//  SYNC
// =============================================================

static rhi::Fence vk_fence_create(bool signaled)
{
    rhi::Fence h = s_fences.alloc();
    if (!h) return {};
    FenceSlot& slot = s_fences.get_checked(h);
    const VkFenceCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0u,
    };
    vkCreateFence(g_ctx.device, &ci, nullptr, &slot.fence);
    return h;
}
static void vk_fence_destroy(rhi::Fence h)
{
    if (FenceSlot* s = s_fences.get(h)) { vkDestroyFence(g_ctx.device, s->fence, nullptr); s_fences.free(h); }
}
static bool vk_fence_wait(const rhi::Fence* hs, u32 n, bool all, u64 timeout)
{
    VkFence fences[32]; n = std::min(n, 32u);
    for (u32 i = 0; i < n; ++i) if (FenceSlot* s = s_fences.get(hs[i])) fences[i] = s->fence;
    return vkWaitForFences(g_ctx.device, n, fences, all ? VK_TRUE : VK_FALSE, timeout) == VK_SUCCESS;
}
static void vk_fence_reset(const rhi::Fence* hs, u32 n)
{
    VkFence fences[32]; n = std::min(n, 32u);
    for (u32 i = 0; i < n; ++i) if (FenceSlot* s = s_fences.get(hs[i])) fences[i] = s->fence;
    vkResetFences(g_ctx.device, n, fences);
}
static bool vk_fence_is_signaled(rhi::Fence h)
{
    FenceSlot* s = s_fences.get(h);
    return s && vkGetFenceStatus(g_ctx.device, s->fence) == VK_SUCCESS;
}

static rhi::Semaphore vk_semaphore_create()
{
    rhi::Semaphore h = s_semaphores.alloc();
    if (!h) return {};
    SemaphoreSlot& slot = s_semaphores.get_checked(h);
    const VkSemaphoreCreateInfo ci{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    vkCreateSemaphore(g_ctx.device, &ci, nullptr, &slot.semaphore);
    return h;
}
static void vk_semaphore_destroy(rhi::Semaphore h)
{
    if (SemaphoreSlot* s = s_semaphores.get(h)) { vkDestroySemaphore(g_ctx.device, s->semaphore, nullptr); s_semaphores.free(h); }
}

// =============================================================
//  SUBMIT
// =============================================================

static void vk_queue_submit(const rhi::CmdBuf* cmds, u32 nc,
                             const rhi::Semaphore* wsems, u32 nw,
                             const rhi::Semaphore* ssems, u32 ns,
                             rhi::Fence signal_fence)
{
    VkCommandBufferSubmitInfo  cb_infos[32];
    VkSemaphoreSubmitInfo      w_infos[16], s_infos[16];
    nc = std::min(nc, 32u); nw = std::min(nw, 16u); ns = std::min(ns, 16u);

    for (u32 i = 0; i < nc; ++i)
    {
        CmdBufSlot* s = s_cmdbufs.get(cmds[i]);
        cb_infos[i] = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                        .commandBuffer = s ? s->cmd : VK_NULL_HANDLE };
    }
    for (u32 i = 0; i < nw; ++i)
    {
        SemaphoreSlot* s = s_semaphores.get(wsems[i]);
        w_infos[i] = { .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                       .semaphore = s ? s->semaphore : VK_NULL_HANDLE,
                       .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT };
    }
    for (u32 i = 0; i < ns; ++i)
    {
        SemaphoreSlot* s = s_semaphores.get(ssems[i]);
        s_infos[i] = { .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                       .semaphore = s ? s->semaphore : VK_NULL_HANDLE,
                       .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT };
    }

    FenceSlot* fs = s_fences.get(signal_fence);
    const VkSubmitInfo2 si{
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount   = nw, .pWaitSemaphoreInfos   = w_infos,
        .commandBufferInfoCount   = nc, .pCommandBufferInfos   = cb_infos,
        .signalSemaphoreInfoCount = ns, .pSignalSemaphoreInfos = s_infos,
    };
    vkQueueSubmit2(g_ctx.graphics_queue, 1, &si, fs ? fs->fence : VK_NULL_HANDLE);
}

static void vk_queue_wait_idle(u32 qf)
{
    VkQueue q = qf == 0 ? g_ctx.graphics_queue : qf == 1 ? g_ctx.compute_queue : g_ctx.transfer_queue;
    vkQueueWaitIdle(q);
}
static void vk_device_wait_idle() { vkDeviceWaitIdle(g_ctx.device); }

// Swapchain is platform-specific; stubs below — implement per platform
static bool vk_swapchain_create(const rhi::SwapchainDesc& desc)
{
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (desc.window_type == rhi::WindowType::Glfw)
    {
        g_ctx.windowHandle = static_cast<GLFWwindow*>(desc.window_handle);
        res = glfwCreateWindowSurface(g_ctx.instance, g_ctx.windowHandle, nullptr, &g_ctx.surface);
    }
#if defined(VK_USE_PLATFORM_WIN32_KHR) || defined(_WIN32)
    else if (desc.window_type == rhi::WindowType::Win32)
    {
        VkWin32SurfaceCreateInfoKHR sci{
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance = static_cast<HINSTANCE>(desc.display_handle ? desc.display_handle : GetModuleHandle(NULL)),
            .hwnd = static_cast<HWND>(desc.window_handle),
        };
        res = vkCreateWin32SurfaceKHR(g_ctx.instance, &sci, nullptr, &g_ctx.surface);
    }
#endif
    else
    {
        fprintf(stderr, "[vkrhi] Unsupported window type: %d\n", (int)desc.window_type);
        return false;
    }

    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "[vkrhi] Failed to create surface (res=%d)\n", res);
        return false;
    }

    // Check WSI support
    VkBool32 supported = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(g_ctx.phys_dev, g_ctx.gfx_family, g_ctx.surface, &supported);
    if (!supported) return false;

    g_ctx.sw_width = desc.width;
    g_ctx.sw_height = desc.height;
    g_ctx.sw_format = VK_FORMAT_B8G8R8A8_SRGB; // Simplified selection
    g_ctx.sw_present_mode = desc.vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;

    const VkSwapchainCreateInfoKHR ci{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = g_ctx.surface,
        .minImageCount = desc.image_count,
        .imageFormat = g_ctx.sw_format,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = { desc.width, desc.height },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = g_ctx.sw_present_mode,
        .clipped = VK_TRUE,
    };

    if (vkCreateSwapchainKHR(g_ctx.device, &ci, nullptr, &g_ctx.swapchain) != VK_SUCCESS)
        return false;

    vkGetSwapchainImagesKHR(g_ctx.device, g_ctx.swapchain, &g_ctx.sw_count, nullptr);
    g_ctx.sw_count = std::min(g_ctx.sw_count, Ctx::MAX_SW_IMAGES);
    vkGetSwapchainImagesKHR(g_ctx.device, g_ctx.swapchain, &g_ctx.sw_count, g_ctx.sw_images);

    for (u32 i = 0; i < g_ctx.sw_count; ++i)
    {
        // Create Texture wrappers for backbuffers
        rhi::Texture h = s_textures.alloc();
        TextureSlot& slot = s_textures.get_checked(h);
        slot.image = g_ctx.sw_images[i];
        slot.format = g_ctx.sw_format;
        slot.width = desc.width;
        slot.height = desc.height;
        slot.is_swapchain = true;
        slot.layout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkImageViewCreateInfo vci{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = slot.image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = slot.format,
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
        };
        vkCreateImageView(g_ctx.device, &vci, nullptr, &slot.view);
        g_ctx.sw_views[i] = slot.view;
        g_ctx.sw_handles[i] = h;
    }

    return true;
}

static void vk_swapchain_destroy()
{
    vkDeviceWaitIdle(g_ctx.device);
    for (u32 i = 0; i < g_ctx.sw_count; ++i)
        s_textures.free(g_ctx.sw_handles[i]); // Wrappers only
    for (u32 i = 0; i < g_ctx.sw_count; ++i)
        vkDestroyImageView(g_ctx.device, g_ctx.sw_views[i], nullptr);
    vkDestroySwapchainKHR(g_ctx.device, g_ctx.swapchain, nullptr);
    vkDestroySurfaceKHR(g_ctx.instance, g_ctx.surface, nullptr);
}

static bool vk_swapchain_acquire(rhi::SwapchainFrame& out, rhi::Semaphore signal)
{
    SemaphoreSlot* s = s_semaphores.get(signal);
    u32 idx = 0;
    VkResult res = vkAcquireNextImageKHR(g_ctx.device, g_ctx.swapchain, ~0ull, s ? s->semaphore : VK_NULL_HANDLE, VK_NULL_HANDLE, &idx);
    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) return false;
    out.index = idx;
    out.backbuffer = g_ctx.sw_handles[idx];
    g_ctx.current_image_index = idx;
    return true;
}

static bool vk_swapchain_present(rhi::Semaphore wait)
{
    SemaphoreSlot* s = s_semaphores.get(wait);
    VkSemaphore wait_sem = s ? s->semaphore : VK_NULL_HANDLE;
    VkPresentInfoKHR pi{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = wait_sem ? 1u : 0u,
        .pWaitSemaphores = wait_sem ? &wait_sem : nullptr,
        .swapchainCount = 1,
        .pSwapchains = &g_ctx.swapchain,
        .pImageIndices = &g_ctx.current_image_index,
    };
    VkResult res = vkQueuePresentKHR(g_ctx.graphics_queue, &pi);
    return res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR;
}

static bool vk_swapchain_resize(u32 w, u32 h)
{
    vk_swapchain_destroy();
    rhi::SwapchainDesc d;
    d.width = w; d.height = h;
    // Re-create with stored window handle (need to store it in Ctx)
    // For this demo we assume fixed size or handle it in app
    return false; 
}

// =============================================================
//  IMGUI
// =============================================================

static void vk_imgui_init()
{
    ImGui_ImplGlfw_InitForVulkan(g_ctx.windowHandle, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = g_ctx.instance;
    init_info.PhysicalDevice = g_ctx.phys_dev;
    init_info.Device = g_ctx.device;
    init_info.QueueFamily = g_ctx.gfx_family;
    init_info.Queue = g_ctx.graphics_queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = g_ctx.desc_pool;
    init_info.RenderPass = VK_NULL_HANDLE;
    init_info.Subpass = 0;
    init_info.MinImageCount = g_ctx.sw_count;
    init_info.ImageCount = g_ctx.sw_count;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.UseDynamicRendering = true;
#ifdef IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING
    init_info.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &g_ctx.sw_format;
#endif
    ImGui_ImplVulkan_Init(&init_info);
}
static VkSampler g_imgui_sampler = VK_NULL_HANDLE;

static void vk_imgui_shutdown()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    if (g_imgui_sampler)
    {
        vkDestroySampler(g_ctx.device, g_imgui_sampler, nullptr);
        g_imgui_sampler = VK_NULL_HANDLE;
    }
}
static void vk_imgui_new_frame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
}
static void vk_imgui_render(rhi::CmdBuf cb)
{
    ImDrawData* draw_data = ImGui::GetDrawData();
    if (!draw_data) return;

    if (CmdBufSlot* s = s_cmdbufs.get(cb))
    {
        ImGui_ImplVulkan_RenderDrawData(draw_data, s->cmd);
    }
}

static void* vk_imgui_add_texture(rhi::Texture h)
{
    TextureSlot* s = s_textures.get(h);
    if (!s) return nullptr;

    if (s->imgui_ds != VK_NULL_HANDLE)
        return (void*)s->imgui_ds;

    if (g_imgui_sampler == VK_NULL_HANDLE)
    {
        VkSamplerCreateInfo info = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vkCreateSampler(g_ctx.device, &info, nullptr, &g_imgui_sampler);
    }

    s->imgui_ds = ImGui_ImplVulkan_AddTexture(g_imgui_sampler, s->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    return (void*)s->imgui_ds;
}

// =============================================================
//  VTABLE
// =============================================================

static const rhi::BackendApi k_api{
    .init                      = vk_init,
    .shutdown                  = vk_shutdown,
    .get_device_info           = vk_get_device_info,
    .swapchain_create          = vk_swapchain_create,
    .swapchain_destroy         = vk_swapchain_destroy,
    .swapchain_acquire         = vk_swapchain_acquire,
    .swapchain_present         = vk_swapchain_present,
    .swapchain_resize          = vk_swapchain_resize,
    .buffer_create             = vk_buffer_create,
    .buffer_destroy            = vk_buffer_destroy,
    .buffer_map                = vk_buffer_map,
    .buffer_unmap              = vk_buffer_unmap,
    .buffer_flush              = vk_buffer_flush,
    .texture_create            = vk_texture_create,
    .texture_destroy           = vk_texture_destroy,
    .sampler_create            = vk_sampler_create,
    .sampler_destroy           = vk_sampler_destroy,
    .shader_create             = vk_shader_create,
    .shader_destroy            = vk_shader_destroy,
    .pipeline_create           = vk_pipeline_create,
    .pipeline_destroy          = vk_pipeline_destroy,
    .descriptor_layout_create  = vk_descriptor_layout_create,
    .descriptor_layout_destroy = vk_descriptor_layout_destroy,
    .descriptor_set_create     = vk_descriptor_set_create,
    .descriptor_set_destroy    = vk_descriptor_set_destroy,
    .descriptor_set_write      = vk_descriptor_set_write,
    .fence_create              = vk_fence_create,
    .fence_destroy             = vk_fence_destroy,
    .fence_wait                = vk_fence_wait,
    .fence_reset               = vk_fence_reset,
    .fence_is_signaled         = vk_fence_is_signaled,
    .semaphore_create          = vk_semaphore_create,
    .semaphore_destroy         = vk_semaphore_destroy,
    .cmdbuf_create             = vk_cmdbuf_create,
    .cmdbuf_destroy            = vk_cmdbuf_destroy,
    .cmdbuf_begin              = vk_cmdbuf_begin,
    .cmdbuf_end                = vk_cmdbuf_end,
    .cmdbuf_reset              = vk_cmdbuf_reset,
    .cmd_begin_render_pass     = vk_cmd_begin_render_pass,
    .cmd_end_render_pass       = vk_cmd_end_render_pass,
    .cmd_set_viewport          = vk_cmd_set_viewport,
    .cmd_set_scissor           = vk_cmd_set_scissor,
    .cmd_bind_pipeline         = vk_cmd_bind_pipeline,
    .cmd_bind_descriptor_set   = vk_cmd_bind_descriptor_set,
    .cmd_bind_vertex_buffers   = vk_cmd_bind_vertex_buffers,
    .cmd_bind_index_buffer     = vk_cmd_bind_index_buffer,
    .cmd_push_constants        = vk_cmd_push_constants,
    .cmd_draw                  = vk_cmd_draw,
    .cmd_draw_indexed          = vk_cmd_draw_indexed,
    .cmd_draw_indirect         = vk_cmd_draw_indirect,
    .cmd_draw_indexed_indirect = vk_cmd_draw_indexed_indirect,
    .cmd_dispatch              = vk_cmd_dispatch,
    .cmd_dispatch_indirect     = vk_cmd_dispatch_indirect,
    .cmd_copy_buffer           = vk_cmd_copy_buffer,
    .cmd_copy_texture          = vk_cmd_copy_texture,
    .cmd_copy_buffer_to_texture= vk_cmd_copy_buffer_to_texture,
    .cmd_copy_texture_to_buffer= vk_cmd_copy_texture_to_buffer,
    .cmd_blit_texture          = vk_cmd_blit_texture,
    .cmd_clear_color_texture   = vk_cmd_clear_color_texture,
    .cmd_clear_depth_texture   = vk_cmd_clear_depth_texture,
    .cmd_fill_buffer           = vk_cmd_fill_buffer,
    .cmd_texture_barrier       = vk_cmd_texture_barrier,
    .cmd_buffer_barrier        = vk_cmd_buffer_barrier,
    .cmd_global_barrier        = vk_cmd_global_barrier,
    .cmd_begin_region          = vk_cmd_begin_region,
    .cmd_end_region            = vk_cmd_end_region,
    .cmd_marker                = vk_cmd_marker,
    .queue_submit              = vk_queue_submit,
    .queue_wait_idle           = vk_queue_wait_idle,
    .device_wait_idle          = vk_device_wait_idle,
    .imgui_init                = vk_imgui_init,
    .imgui_shutdown            = vk_imgui_shutdown,
    .imgui_new_frame           = vk_imgui_new_frame,
    .imgui_render              = vk_imgui_render,
    .imgui_add_texture         = vk_imgui_add_texture,
};

const rhi::BackendApi* get_api() noexcept { return &k_api; }

} // namespace vkrhi
