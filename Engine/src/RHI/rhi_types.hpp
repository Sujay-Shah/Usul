#pragma once
// =============================================================
//  rhi_types.hpp — Public RHI type definitions
//
//  Rules:
//    • No backend headers included here (no vulkan.h, no d3d12.h)
//    • No implementation — pure data declarations
//    • All structs are trivially copyable and zero-initialised by default
//    • Bitflag enums provide operator| / operator& so callers never
//      cast to u32 manually
// =============================================================

#include <cstdint>
#include <cstddef>
#include <cassert>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i32 = int32_t;
using f32 = float;

namespace rhi {

// =============================================================
//  HANDLE SYSTEM
//
//  32-bit packed value:
//    bits [19: 0] — slot index   (up to 1 048 576 live objects)
//    bits [31:20] — generation   (4 096 reuses before wrap)
//
//  Value 0 is permanently invalid (null handle).
//
//  Each resource type has its own Handle<Tag> so that
//  passing a Texture where a Buffer is expected is a
//  compile error, not a silent runtime bug.
// =============================================================

inline constexpr u32 HANDLE_INDEX_BITS = 20u;
inline constexpr u32 HANDLE_GEN_BITS   = 12u;
inline constexpr u32 HANDLE_INDEX_MASK = (1u << HANDLE_INDEX_BITS) - 1u;
inline constexpr u32 HANDLE_GEN_MASK   = (1u << HANDLE_GEN_BITS)  - 1u;

template<typename Tag>
struct Handle
{
    u32 id = 0;

    // Explicit bool — must write `if (h)` not `if (h.id)`
    explicit constexpr operator bool() const noexcept { return id != 0; }

    constexpr bool operator==(Handle o) const noexcept { return id == o.id; }
    constexpr bool operator!=(Handle o) const noexcept { return id != o.id; }
    constexpr bool operator< (Handle o) const noexcept { return id <  o.id; } // map key

    [[nodiscard]] constexpr u32 index() const noexcept { return  id        & HANDLE_INDEX_MASK; }
    [[nodiscard]] constexpr u32 gen()   const noexcept { return (id >> HANDLE_INDEX_BITS) & HANDLE_GEN_MASK; }

    static constexpr Handle null() noexcept { return {}; }
};

// One tag type per resource kind — never instantiated
struct BufferTag          {};
struct TextureTag         {};
struct SamplerTag         {};
struct ShaderTag          {};
struct PipelineTag        {};
struct CmdBufTag          {};
struct FenceTag           {};
struct SemaphoreTag       {};
struct DescriptorSetTag   {};
struct DescriptorLayoutTag{};

using Buffer           = Handle<BufferTag>;
using Texture          = Handle<TextureTag>;
using Sampler          = Handle<SamplerTag>;
using Shader           = Handle<ShaderTag>;
using Pipeline         = Handle<PipelineTag>;
using CmdBuf           = Handle<CmdBufTag>;
using Fence            = Handle<FenceTag>;
using Semaphore        = Handle<SemaphoreTag>;
using DescriptorSet    = Handle<DescriptorSetTag>;
using DescriptorLayout = Handle<DescriptorLayoutTag>;

// =============================================================
//  LIMITS  (pool capacities — tuned for a mid-range game)
// =============================================================

inline constexpr u32 MAX_BUFFERS             = 1u << HANDLE_INDEX_BITS; // 1 048 576
inline constexpr u32 MAX_TEXTURES            = 1u << HANDLE_INDEX_BITS;
inline constexpr u32 MAX_SAMPLERS            = 512;
inline constexpr u32 MAX_SHADERS             = 2048;
inline constexpr u32 MAX_PIPELINES           = 2048;
inline constexpr u32 MAX_CMDBUFS             = 256;
inline constexpr u32 MAX_FENCES              = 256;
inline constexpr u32 MAX_SEMAPHORES          = 256;
inline constexpr u32 MAX_DESCRIPTOR_SETS     = 8192;
inline constexpr u32 MAX_DESC_LAYOUTS        = 512;
inline constexpr u32 MAX_COLOR_TARGETS       = 8;
inline constexpr u32 MAX_VERTEX_ATTRIBS      = 16;
inline constexpr u32 MAX_VERTEX_BUFFERS      = 8;
inline constexpr u32 MAX_DESCRIPTOR_BINDINGS = 32;
inline constexpr u32 MAX_FRAMES_IN_FLIGHT    = 3;

// =============================================================
//  ENUMS
// =============================================================

enum class Backend  : u32 { Vulkan, DX12, Count };

enum class Format   : u32
{
    Undefined = 0,
    // 8-bit
    R8_Unorm, R8_Snorm, R8_Uint, R8_Sint,
    RG8_Unorm,
    RGBA8_Unorm, RGBA8_Snorm, RGBA8_Srgb,
    BGRA8_Unorm, BGRA8_Srgb,
    // 16-bit float
    R16_Float, RG16_Float, RGBA16_Float,
    // 16-bit uint / int
    R16_Uint, R16_Sint,
    // 32-bit float
    R32_Float, RG32_Float, RGB32_Float, RGBA32_Float,
    // 32-bit uint
    R32_Uint, RG32_Uint, RGBA32_Uint,
    // packed
    RGB10A2_Unorm, RG11B10_Float, RGB9E5_Float,
    // depth / stencil
    D16_Unorm,
    D24_Unorm_S8_Uint,
    D32_Float,
    D32_Float_S8_Uint,
    // BC compressed
    BC1_Unorm, BC1_Srgb,
    BC2_Unorm, BC2_Srgb,
    BC3_Unorm, BC3_Srgb,
    BC4_Unorm, BC4_Snorm,
    BC5_Unorm, BC5_Snorm,
    BC6H_UFloat, BC6H_SFloat,
    BC7_Unorm, BC7_Srgb,
    Count
};

// Bitflag helper macro — generates operator|, operator&, operator~, any()
#define RHI_DEFINE_FLAGS(T)                                                    \
    inline T operator|(T a, T b)  { return static_cast<T>(static_cast<u32>(a) | static_cast<u32>(b)); } \
    inline T operator&(T a, T b)  { return static_cast<T>(static_cast<u32>(a) & static_cast<u32>(b)); } \
    inline T operator^(T a, T b)  { return static_cast<T>(static_cast<u32>(a) ^ static_cast<u32>(b)); } \
    inline T operator~(T a)       { return static_cast<T>(~static_cast<u32>(a)); }                      \
    inline T& operator|=(T& a, T b) { a = a | b; return a; }                  \
    inline T& operator&=(T& a, T b) { a = a & b; return a; }                  \
    inline bool any(T f)          { return static_cast<u32>(f) != 0; }        \
    inline bool has(T flags, T bit){ return any(flags & bit); }

enum class BufferUsage : u32
{
    None        = 0,
    Vertex      = 1 << 0,
    Index       = 1 << 1,
    Uniform     = 1 << 2,
    Storage     = 1 << 3,
    Indirect    = 1 << 4,
    TransferSrc = 1 << 5,
    TransferDst = 1 << 6,
};
RHI_DEFINE_FLAGS(BufferUsage)

enum class TextureUsage : u32
{
    None        = 0,
    Sampled     = 1 << 0,
    Storage     = 1 << 1,
    ColorTarget = 1 << 2,
    DepthTarget = 1 << 3,
    TransferSrc = 1 << 4,
    TransferDst = 1 << 5,
};
RHI_DEFINE_FLAGS(TextureUsage)

enum class ShaderStage : u32
{
    None     = 0,
    Vertex   = 1 << 0,
    Fragment = 1 << 1,
    Compute  = 1 << 2,
    All      = Vertex | Fragment | Compute,
};
RHI_DEFINE_FLAGS(ShaderStage)

enum class PipelineStage : u32
{
    None        = 0,
    Top         = 1 << 0,
    DrawIndirect= 1 << 1,
    Vertex      = 1 << 2,
    EarlyDepth  = 1 << 3,
    Fragment    = 1 << 4,
    LateDepth   = 1 << 5,
    ColorOutput = 1 << 6,
    Compute     = 1 << 7,
    Transfer    = 1 << 8,
    Bottom      = 1 << 9,
    AllGraphics = Vertex | EarlyDepth | Fragment | LateDepth | ColorOutput,
    All         = 0x3FF,
};
RHI_DEFINE_FLAGS(PipelineStage)

enum class Access : u32
{
    None          = 0,
    IndirectRead  = 1 << 0,
    IndexRead     = 1 << 1,
    VertexRead    = 1 << 2,
    UniformRead   = 1 << 3,
    ShaderRead    = 1 << 4,
    ShaderWrite   = 1 << 5,
    ColorRead     = 1 << 6,
    ColorWrite    = 1 << 7,
    DepthRead     = 1 << 8,
    DepthWrite    = 1 << 9,
    TransferRead  = 1 << 10,
    TransferWrite = 1 << 11,
    MemoryRead    = 1 << 12,
    MemoryWrite   = 1 << 13,
};
RHI_DEFINE_FLAGS(Access)

#undef RHI_DEFINE_FLAGS

enum class MemoryType : u32
{
    GpuOnly,      // VRAM — device-local, no CPU access
    CpuToGpu,     // Upload heap / staging — write-combine
    GpuToCpu,     // Readback heap — cached + coherent
    CpuCoherent,  // Persistently mapped, write-combine for UBOs
};

enum class TextureDim : u32 { D1, D2, D3, Cube, D2Array, CubeArray };

enum class LoadOp  : u32 { Load, Clear, DontCare };
enum class StoreOp : u32 { Store, DontCare };

enum class IndexType         : u32 { Uint16, Uint32 };
enum class PrimitiveTopology : u32 { TriangleList, TriangleStrip, LineList, PointList };
enum class CullMode          : u32 { None, Front, Back };
enum class FrontFace         : u32 { CCW, CW };
enum class FillMode          : u32 { Solid, Wireframe };

enum class CompareOp : u32
    { Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always };

enum class StencilOp : u32
    { Keep, Zero, Replace, IncrClamp, DecrClamp, Invert, IncrWrap, DecrWrap };

enum class BlendFactor : u32
{
    Zero, One,
    SrcColor, OneMinusSrcColor,
    DstColor, OneMinusDstColor,
    SrcAlpha, OneMinusSrcAlpha,
    DstAlpha, OneMinusDstAlpha,
    SrcAlphaSaturate,
    ConstantColor, OneMinusConstantColor,
};

enum class BlendOp : u32 { Add, Subtract, ReverseSubtract, Min, Max };

enum class VertexInputRate  : u32 { Vertex, Instance };

enum class DescriptorType : u32
{
    UniformBuffer,
    UniformBufferDynamic,
    StorageBuffer,
    StorageBufferDynamic,
    SampledTexture,
    StorageTexture,
    Sampler,
    CombinedImageSampler,
    InputAttachment,
};

enum class TextureLayout : u32
{
    Undefined,
    General,
    ColorTarget,
    DepthStencilTarget,
    DepthStencilReadOnly,
    ShaderReadOnly,
    TransferSrc,
    TransferDst,
    Present,
};

enum class SamplerFilter   : u32 { Nearest, Linear };
enum class SamplerMipmap   : u32 { Nearest, Linear };
enum class SamplerAddress  : u32 { Repeat, MirroredRepeat, ClampToEdge, ClampToBorder, MirrorClampToEdge };
enum class BorderColor     : u32 { TransparentBlack, OpaqueBlack, OpaqueWhite };

// =============================================================
//  CREATION DESCRIPTORS
//  Plain-data structs with member defaults — use designated
//  initializers (C++20) for readable call sites:
//
//    rhi::BufferDesc vb{
//        .size   = sizeof(verts),
//        .usage  = rhi::BufferUsage::Vertex | rhi::BufferUsage::TransferDst,
//        .memory = rhi::MemoryType::GpuOnly,
//        .name   = "mesh_vb",
//    };
// =============================================================

struct BufferDesc
{
    u64         size   = 0;
    BufferUsage usage  = BufferUsage::None;
    MemoryType  memory = MemoryType::GpuOnly;
    const char *name   = nullptr;
};

struct TextureDesc
{
    u32          width        = 1;
    u32          height       = 1;
    u32          depth        = 1;
    u32          mip_levels   = 1;
    u32          array_layers = 1;
    u32          sample_count = 1;
    Format       format       = Format::Undefined;
    TextureDim   dim          = TextureDim::D2;
    TextureUsage usage        = TextureUsage::None;
    const char  *name         = nullptr;
};

struct SamplerDesc
{
    SamplerFilter  min_filter  = SamplerFilter::Linear;
    SamplerFilter  mag_filter  = SamplerFilter::Linear;
    SamplerMipmap  mip_mode    = SamplerMipmap::Linear;
    SamplerAddress address_u   = SamplerAddress::Repeat;
    SamplerAddress address_v   = SamplerAddress::Repeat;
    SamplerAddress address_w   = SamplerAddress::Repeat;
    BorderColor    border      = BorderColor::OpaqueBlack;
    f32            mip_bias    = 0.f;
    f32            min_lod     = 0.f;
    f32            max_lod     = 1000.f;
    bool           anisotropy  = false;
    f32            max_aniso   = 1.f;
    bool           compare     = false;
    CompareOp      compare_op  = CompareOp::Never;
    bool           unnorm_coords = false;
    const char    *name        = nullptr;
};

struct ShaderDesc
{
    const void  *bytecode  = nullptr;   // SPIR-V or DXIL
    u64          size      = 0;
    ShaderStage  stage     = ShaderStage::None;
    const char  *entry     = "main";
    const char  *name      = nullptr;
};

// ---- Pipeline sub-structs ----

struct VertexAttrib
{
    u32    location = 0;
    u32    binding  = 0;
    Format format   = Format::Undefined;
    u32    offset   = 0;
};

struct VertexBinding
{
    u32             binding    = 0;
    u32             stride     = 0;
    VertexInputRate input_rate       = VertexInputRate::Vertex;
};

struct BlendState
{
    bool        enable       = false;
    BlendFactor src_color    = BlendFactor::SrcAlpha;
    BlendFactor dst_color    = BlendFactor::OneMinusSrcAlpha;
    BlendOp     color_op     = BlendOp::Add;
    BlendFactor src_alpha    = BlendFactor::One;
    BlendFactor dst_alpha    = BlendFactor::Zero;
    BlendOp     alpha_op     = BlendOp::Add;
    u8          write_mask   = 0xF; // RGBA
};

struct DepthState
{
    bool      test_write  = true;   // shorthand: test=true write=true compare=Less
    bool      test_enable = true;
    bool      write_enable= true;
    CompareOp compare_op  = CompareOp::Less;
    bool      bounds_test = false;
    f32       min_bounds  = 0.f;
    f32       max_bounds  = 1.f;
};

struct StencilOpState
{
    StencilOp fail_op       = StencilOp::Keep;
    StencilOp depth_fail_op = StencilOp::Keep;
    StencilOp pass_op       = StencilOp::Keep;
    CompareOp compare_op    = CompareOp::Always;
    u32       compare_mask  = 0xFF;
    u32       write_mask    = 0xFF;
    u32       reference     = 0;
};

struct StencilState
{
    bool         enable = false;
    StencilOpState front = {};
    StencilOpState back  = {};
};

struct RasterState
{
    CullMode  cull_mode       = CullMode::Back;
    FrontFace front_face = FrontFace::CCW;
    FillMode  fill_mode       = FillMode::Solid;
    f32       depth_bias            = 0.f;
    f32       depth_bias_slope      = 0.f;
    f32       depth_bias_clamp      = 0.f;
    bool      depth_clip_enable     = true;
    bool      conservative_raster   = false;
    bool      alpha_to_coverage     = false;
};

struct PipelineDesc
{
    // Shaders
    Shader vertex_shader   = {};
    Shader fragment_shader = {};
    Shader compute_shader  = {};   // set for compute pipelines

    // Descriptor layout (set 0 for now; extend for multiple sets)
    DescriptorLayout layout = {};

    // Vertex input
    VertexAttrib  attribs [MAX_VERTEX_ATTRIBS] = {};
    u32           attrib_count = 0;
    VertexBinding bindings[MAX_VERTEX_BUFFERS] = {};
    u32           binding_count = 0;

    // Render target formats (for dynamic rendering / pipeline creation)
    Format color_formats[MAX_COLOR_TARGETS] = {};
    u32    color_count   = 0;
    Format depth_format  = Format::Undefined;
    Format stencil_format= Format::Undefined;

    // Fixed-function state
    BlendState          blend  [MAX_COLOR_TARGETS] = {};
    DepthState          depth                      = {};
    StencilState        stencil                    = {};
    RasterState         raster                     = {};
    PrimitiveTopology   topology                   = PrimitiveTopology::TriangleList;
    u32                 sample_count               = 1;
    u32                 patch_control_points       = 0; // for tessellation

    // Push constant range (single range shared across all stages)
    u32 push_constant_size   = 0;
    u32 push_constant_offset = 0;

    const char *name = nullptr;
};

struct DescriptorBinding
{
    u32            binding     = 0;
    DescriptorType type        = DescriptorType::UniformBuffer;
    u32            count       = 1;
    ShaderStage    stages      = ShaderStage::All;
};

struct DescriptorLayoutDesc
{
    DescriptorBinding bindings[MAX_DESCRIPTOR_BINDINGS] = {};
    u32               count    = 0;
    bool              bindless = false; // for descriptor indexing
    const char       *name     = nullptr;
};

// =============================================================
//  RENDER PASS
// =============================================================

struct ClearColor   { f32 r = 0.f, g = 0.f, b = 0.f, a = 1.f; };
struct ClearDepth   { f32 depth = 1.f; u8 stencil = 0; };

struct ColorAttachment
{
    Texture    texture         = {};
    Texture    resolve_texture = {};  // MSAA resolve; null if unused
    u32        mip             = 0;
    u32        layer           = 0;
    LoadOp     load_op         = LoadOp::Clear;
    StoreOp    store_op        = StoreOp::Store;
    TextureLayout layout       = TextureLayout::ColorTarget;
    ClearColor clear           = {};
};

struct DepthAttachment
{
    Texture    texture          = {};
    LoadOp     load_op          = LoadOp::Clear;
    StoreOp    store_op         = StoreOp::DontCare;
    LoadOp     stencil_load_op  = LoadOp::DontCare;
    StoreOp    stencil_store_op = StoreOp::DontCare;
    TextureLayout layout        = TextureLayout::DepthStencilTarget;
    ClearDepth clear            = {};
};

struct RenderPassDesc
{
    ColorAttachment color[MAX_COLOR_TARGETS] = {};
    u32             color_count = 0;
    DepthAttachment depth       = {};
    bool            has_depth   = false;
    // Viewport/scissor set separately via cmd_set_viewport / cmd_set_scissor
};

// =============================================================
//  BARRIERS
// =============================================================

struct TextureBarrier
{
    Texture       tex         = {};
    TextureLayout old_layout  = TextureLayout::Undefined;
    TextureLayout new_layout  = TextureLayout::Undefined;
    PipelineStage src_stage   = PipelineStage::Top;
    PipelineStage dst_stage   = PipelineStage::Bottom;
    Access        src_access  = Access::None;
    Access        dst_access  = Access::None;
    u32           base_mip    = 0;
    u32           mip_count   = 0;   // 0 = all remaining
    u32           base_layer  = 0;
    u32           layer_count = 0;   // 0 = all remaining
};

struct BufferBarrier
{
    Buffer        buf        = {};
    PipelineStage src_stage  = PipelineStage::Top;
    PipelineStage dst_stage  = PipelineStage::Bottom;
    Access        src_access = Access::None;
    Access        dst_access = Access::None;
    u64           offset     = 0;
    u64           size       = 0;    // 0 = whole buffer
};

// =============================================================
//  DESCRIPTOR WRITES
// =============================================================

struct DescriptorWrite
{
    u32            binding     = 0;
    u32            array_index = 0;
    DescriptorType type        = DescriptorType::UniformBuffer;

    // Only one of these is valid per write, determined by type
    struct BufferInfo
    {
        Buffer buf    = {};
        u64    offset = 0;
        u64    range  = ~0ull; // VK_WHOLE_SIZE
    };
    struct TextureInfo
    {
        Texture       tex    = {};
        Sampler       samp   = {};
        TextureLayout layout = TextureLayout::ShaderReadOnly;
    };

    BufferInfo  buffer  = {};
    TextureInfo texture = {};
};

// =============================================================
//  COMMANDS
// =============================================================

struct Viewport
{
    f32 x = 0.f, y = 0.f;
    f32 w = 0.f, h = 0.f;
    f32 min_depth = 0.f, max_depth = 1.f;
};

struct Rect { i32 x = 0, y = 0; u32 w = 0, h = 0; };

struct BufferCopy
{
    Buffer src = {}; u64 src_offset = 0;
    Buffer dst = {}; u64 dst_offset = 0;
    u64    size = 0;
};

struct TextureCopy
{
    Texture src       = {}; u32 src_mip = 0, src_layer = 0;
    Texture dst       = {}; u32 dst_mip = 0, dst_layer = 0;
    u32 src_x = 0, src_y = 0, src_z = 0;
    u32 dst_x = 0, dst_y = 0, dst_z = 0;
    u32 width = 0, height = 0, depth = 1;
};

struct BufferTextureCopy
{
    Buffer  buf          = {};
    u64     buf_offset   = 0;
    u32     buf_row_len  = 0;   // 0 = tightly packed
    u32     buf_img_h    = 0;
    Texture tex          = {};
    u32     mip          = 0;
    u32     base_layer   = 0;
    u32     layer_count  = 1;
    u32     x = 0, y = 0, z = 0;
    u32     w = 0, h = 0, d = 1;
};

struct DrawCmd
{
    u32 vertex_count   = 0;
    u32 instance_count = 1;
    u32 first_vertex   = 0;
    u32 first_instance = 0;
};

struct DrawIndexedCmd
{
    u32 index_count    = 0;
    u32 instance_count = 1;
    u32 first_index    = 0;
    i32 vertex_offset  = 0;
    u32 first_instance = 0;
};

struct DispatchCmd { u32 x = 1, y = 1, z = 1; };

// =============================================================
//  SWAPCHAIN
// =============================================================

struct SwapchainDesc
{
    void       *window_handle  = nullptr; // HWND / xcb_window_t / NSWindow*
    void       *display_handle = nullptr; // HINSTANCE / xcb_connection_t / nullptr
    u32         width          = 0;
    u32         height         = 0;
    u32         image_count    = 3;
    Format      format         = Format::BGRA8_Srgb;
    bool        vsync          = true;
};

struct SwapchainFrame
{
    Texture backbuffer = {};
    u32     index      = 0;
};

// =============================================================
//  INIT / DEVICE
// =============================================================

struct InitDesc
{
    Backend     backend               = Backend::Vulkan;
    bool        validation            = false;
    bool        gpu_crash_dumps       = false;
    const char *app_name              = "rhi_app";
    u32         app_version           = 0;
};

struct DeviceInfo
{
    char  name[256]              = {};
    u64   vram_bytes             = 0;
    u64   shared_system_ram      = 0;
    u32   max_push_constant_size = 0;
    u32   max_anisotropy         = 0;
    bool  raytracing             = false;
    bool  mesh_shaders           = false;
    bool  variable_rate_shading  = false;
    bool  bindless               = false;
};

// Returned by buffer_map; call buffer_unmap when done
struct MappedBuffer
{
    void   *ptr    = nullptr;
    u64     size   = 0;
    Buffer  buffer = {};
};

} // namespace rhi
