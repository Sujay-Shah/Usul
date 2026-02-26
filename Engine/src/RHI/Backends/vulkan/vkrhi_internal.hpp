#pragma once
// =============================================================
//  vkrhi_internal.hpp — Vulkan backend private types
//
//  NEVER include this from application code.
//  Include rhi.hpp instead.
//
//  Production notes
//  ----------------
//  • VMA is used for all sub-allocations (one VmaAllocator per
//    device; no per-resource vkAllocateMemory calls).
//  • VkPipelineLayout is derived from the DescriptorLayout handle
//    stored in PipelineSlot so we can call vkCmdPushConstants
//    with the correct layout at draw time.
//  • Command pools are per-queue-family and reset per-frame via
//    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT.
//  • Swapchain images are wrapped in TextureSlots with
//    is_swapchain=true so the shutdown for_each skips them.
//  • All VkSemaphore and VkFence slots own the object; the Pool
//    generation counter catches double-free.
//  • Debug names propagated via VK_EXT_debug_utils (loaded
//    dynamically; noop when the extension is absent).
// =============================================================

#include "../../rhi_pool.hpp"   // rhi::Pool, rhi::SlotHeader
#include "../../rhi.hpp"        // rhi::BackendApi + all public types

// VMA must be compiled in exactly one .cpp — define this before
// including vk_mem_alloc.h there:
   #define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>       // https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator

#include <vulkan/vulkan.h>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <algorithm>            // std::min / max

struct GLFWwindow;

namespace vkrhi {

using u8  = ::u8;
using u16 = ::u16;
using u32 = ::u32;
using u64 = ::u64;
using f32 = ::f32;

// =============================================================
//  CONVERSION TABLES  (declared here, defined in vkrhi.cpp)
// =============================================================
VkFormat             to_vk_format       (rhi::Format f) noexcept;
VkImageLayout        to_vk_layout       (rhi::TextureLayout l) noexcept;
VkAccessFlags2       to_vk_access2      (rhi::Access a) noexcept;
VkPipelineStageFlags2 to_vk_stage2      (rhi::PipelineStage s) noexcept;
VkShaderStageFlags   to_vk_shader_stage (rhi::ShaderStage s) noexcept;
VkDescriptorType     to_vk_desc_type    (rhi::DescriptorType t) noexcept;
VkCompareOp          to_vk_compare_op   (rhi::CompareOp c) noexcept;
VkFilter             to_vk_filter       (rhi::SamplerFilter f) noexcept;
VkSamplerMipmapMode  to_vk_mipmap       (rhi::SamplerMipmap m) noexcept;
VkSamplerAddressMode to_vk_address      (rhi::SamplerAddress a) noexcept;
VkBlendFactor        to_vk_blend_factor (rhi::BlendFactor b) noexcept;
VkBlendOp            to_vk_blend_op     (rhi::BlendOp o) noexcept;
VkCullModeFlagBits   to_vk_cull         (rhi::CullMode c) noexcept;
VkFrontFace          to_vk_front_face   (rhi::FrontFace f) noexcept;
VkPolygonMode        to_vk_fill         (rhi::FillMode f) noexcept;
VkPrimitiveTopology  to_vk_topology     (rhi::PrimitiveTopology t) noexcept;
VmaMemoryUsage       to_vma_usage       (rhi::MemoryType m) noexcept;
VkImageAspectFlags   aspect_for_format  (VkFormat f) noexcept;

// =============================================================
//  DEVICE CONTEXT
//  One global per process — Vulkan only allows one logical device
//  for the common single-GPU case.  Multi-GPU would need a ctx array.
// =============================================================
struct Ctx
{
    VkInstance               instance        = VK_NULL_HANDLE;
    VkPhysicalDevice         phys_dev        = VK_NULL_HANDLE;
    VkDevice                 device          = VK_NULL_HANDLE;
    VmaAllocator             allocator       = VK_NULL_HANDLE;

    // Queues — all three may be the same queue family on integrated GPUs
    VkQueue  graphics_queue = VK_NULL_HANDLE;
    VkQueue  compute_queue  = VK_NULL_HANDLE;
    VkQueue  transfer_queue = VK_NULL_HANDLE;
    u32      gfx_family     = ~0u;
    u32      comp_family    = ~0u;
    u32      xfr_family     = ~0u;

    // One command pool per queue family; reset per command buffer
    VkCommandPool gfx_pool  = VK_NULL_HANDLE;
    VkCommandPool comp_pool = VK_NULL_HANDLE;
    VkCommandPool xfr_pool  = VK_NULL_HANDLE;

    // Global descriptor pool — sized conservatively; adjust for your project
    VkDescriptorPool desc_pool = VK_NULL_HANDLE;

    // Physical device properties for validation / alignment queries
    VkPhysicalDeviceProperties       props       = {};
    VkPhysicalDeviceVulkan12Features feats12     = {};
    VkPhysicalDeviceVulkan13Features feats13     = {};

    // Swapchain
    GLFWwindow*              windowHandle = nullptr;
    VkSurfaceKHR             surface         = VK_NULL_HANDLE;
    VkSwapchainKHR           swapchain       = VK_NULL_HANDLE;
    static constexpr u32     MAX_SW_IMAGES   = 8;
    VkImage                  sw_images  [MAX_SW_IMAGES] = {};
    VkImageView              sw_views   [MAX_SW_IMAGES] = {};
    rhi::Texture             sw_handles [MAX_SW_IMAGES] = {}; // TextureSlot wrappers
    u32                      sw_count        = 0;
    u32                      sw_width        = 0;
    u32                      sw_height       = 0;
    VkFormat                 sw_format       = VK_FORMAT_UNDEFINED;
    VkPresentModeKHR         sw_present_mode = VK_PRESENT_MODE_FIFO_KHR;

    // Debug utils extension function pointers (null if extension absent)
    VkDebugUtilsMessengerEXT          debug_messenger  = VK_NULL_HANDLE;
    PFN_vkCmdBeginDebugUtilsLabelEXT  pfn_begin_label  = nullptr;
    PFN_vkCmdEndDebugUtilsLabelEXT    pfn_end_label    = nullptr;
    PFN_vkCmdInsertDebugUtilsLabelEXT pfn_insert_label = nullptr;
    PFN_vkSetDebugUtilsObjectNameEXT  pfn_set_name     = nullptr;

    bool validation_enabled = false;

    // Minimum UBO alignment (use this to pad dynamic UBO offsets)
    u32  min_ubo_align = 256;
};

extern Ctx g_ctx;

// =============================================================
//  RESOURCE SLOTS
//  Each slot holds: rhi::SlotHeader (generation + alive flag)
//  PLUS backend-specific Vulkan objects.
//
//  The SlotHeader MUST be the first member — enforced by
//  Pool<>::static_assert(offsetof(T, hdr) == 0).
// =============================================================

struct BufferSlot
{
    rhi::SlotHeader hdr;                // FIRST MEMBER — required
    VkBuffer        buffer      = VK_NULL_HANDLE;
    VmaAllocation   allocation  = VK_NULL_HANDLE; // VMA owns the memory
    VkDeviceAddress gpu_address = 0;              // BDA if enabled
    VkDeviceSize    size        = 0;
    void           *mapped_ptr  = nullptr;        // non-null for CpuToGpu/CpuCoherent
    rhi::BufferUsage usage      = rhi::BufferUsage::None;
};

struct TextureSlot
{
    rhi::SlotHeader  hdr;
    VkImage          image       = VK_NULL_HANDLE;
    VkImageView      view        = VK_NULL_HANDLE; // default full-range view
    VmaAllocation    allocation  = VK_NULL_HANDLE; // null for swapchain images
    VkFormat         format      = VK_FORMAT_UNDEFINED;
    VkImageLayout    layout      = VK_IMAGE_LAYOUT_UNDEFINED; // tracked current layout
    u32              width       = 0;
    u32              height      = 0;
    u32              depth       = 1;
    u32              mip_levels  = 1;
    u32              array_layers= 1;
    u32              sample_count= 1;
    bool             is_swapchain= false; // skip VMA free on destroy
};

struct SamplerSlot
{
    rhi::SlotHeader hdr;
    VkSampler       sampler = VK_NULL_HANDLE;
};

struct ShaderSlot
{
    rhi::SlotHeader  hdr;
    VkShaderModule   module  = VK_NULL_HANDLE;
    rhi::ShaderStage stage   = rhi::ShaderStage::None;
    char             entry[64] = {};
};

struct DescLayoutSlot
{
    rhi::SlotHeader       hdr;
    VkDescriptorSetLayout layout          = VK_NULL_HANDLE;
    VkPipelineLayout      pipeline_layout = VK_NULL_HANDLE; // derived from layout + push range
    u32                   push_size       = 0;
};

struct DescSetSlot
{
    rhi::SlotHeader  hdr;
    VkDescriptorSet  set    = VK_NULL_HANDLE;
    rhi::DescriptorLayout layout_handle = {}; // back-reference for validation
};

struct PipelineSlot
{
    rhi::SlotHeader     hdr;
    VkPipeline          pipeline       = VK_NULL_HANDLE;
    VkPipelineLayout    pipeline_layout= VK_NULL_HANDLE; // borrowed from DescLayoutSlot
    VkPipelineBindPoint bind_point     = VK_PIPELINE_BIND_POINT_GRAPHICS;
};

struct CmdBufSlot
{
    rhi::SlotHeader hdr;
    VkCommandBuffer cmd          = VK_NULL_HANDLE;
    u32             queue_family = 0;
    bool            recording    = false;
};

struct FenceSlot
{
    rhi::SlotHeader hdr;
    VkFence         fence = VK_NULL_HANDLE;
};

struct SemaphoreSlot
{
    rhi::SlotHeader hdr;
    VkSemaphore     semaphore = VK_NULL_HANDLE;
};

// =============================================================
//  POOLS
//  Declared extern here; defined once in vkrhi.cpp as file-scope
//  statics.  Large pools (Buffers, Textures) MUST be static to
//  avoid blowing the stack (~128 MB for 1M buffer slots).
// =============================================================

extern rhi::Pool<BufferSlot,     rhi::MAX_BUFFERS,         rhi::Buffer>&          g_buffers;
extern rhi::Pool<TextureSlot,    rhi::MAX_TEXTURES,        rhi::Texture>&         g_textures;
extern rhi::Pool<SamplerSlot,    rhi::MAX_SAMPLERS,        rhi::Sampler>&         g_samplers;
extern rhi::Pool<ShaderSlot,     rhi::MAX_SHADERS,         rhi::Shader>&          g_shaders;
extern rhi::Pool<DescLayoutSlot, rhi::MAX_DESC_LAYOUTS,    rhi::DescriptorLayout>& g_desc_layouts;
extern rhi::Pool<DescSetSlot,    rhi::MAX_DESCRIPTOR_SETS, rhi::DescriptorSet>&   g_desc_sets;
extern rhi::Pool<PipelineSlot,   rhi::MAX_PIPELINES,       rhi::Pipeline>&        g_pipelines;
extern rhi::Pool<CmdBufSlot,     rhi::MAX_CMDBUFS,         rhi::CmdBuf>&          g_cmdbufs;
extern rhi::Pool<FenceSlot,      rhi::MAX_FENCES,          rhi::Fence>&           g_fences;
extern rhi::Pool<SemaphoreSlot,  rhi::MAX_SEMAPHORES,      rhi::Semaphore>&       g_semaphores;

// =============================================================
//  INTERNAL HELPERS
// =============================================================

// Name a Vulkan object so it shows up in RenderDoc / Nsight
void set_debug_name(VkObjectType type, u64 handle, const char* name) noexcept;

// Pad 'offset' up to the required UBO alignment
inline u32 align_ubo(u32 offset) noexcept
{
    const u32 a = g_ctx.min_ubo_align - 1;
    return (offset + a) & ~a;
}

// =============================================================
//  BACKEND ENTRY POINT
// =============================================================

const rhi::BackendApi* get_api() noexcept;

} // namespace vkrhi
