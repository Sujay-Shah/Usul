#pragma once
// =============================================================
//  rhi.hpp — Public RHI API
//
//  Single header the application includes.  Never include
//  any backend headers (vulkan.h, d3d12.h) from application code.
//
//  Dispatch model
//  --------------
//  g_rhi is a pointer to a const BackendApi vtable — one static
//  instance per backend.  Each free function below is a one-liner
//  that goes through g_rhi; in release builds the compiler sees
//  a single load + indirect call and can hoist the load.
//
//  Frame pacing helpers (FrameContext) are declared here so
//  application code can write backend-agnostic frame loops.
// =============================================================

#include "rhi_types.hpp"

namespace rhi {

// =============================================================
//  BACKEND VTABLE
//  A plain struct of function pointers — zero virtual dispatch
//  overhead, one pointer indirection, easily mocked in tests.
// =============================================================

struct BackendApi
{
    // ---- Device ----
    bool (*init)          (const InitDesc&);
    void (*shutdown)      ();
    void (*get_device_info)(DeviceInfo&);

    // ---- Swapchain ----
    bool (*swapchain_create) (const SwapchainDesc&);
    void (*swapchain_destroy)();
    bool (*swapchain_acquire)(SwapchainFrame&, Semaphore signal_sem);
    bool (*swapchain_present)(Semaphore wait_sem);
    bool (*swapchain_resize) (u32 w, u32 h);

    // ---- Buffers ----
    Buffer       (*buffer_create) (const BufferDesc&);
    void         (*buffer_destroy)(Buffer);
    MappedBuffer (*buffer_map)    (Buffer);
    void         (*buffer_unmap)  (Buffer);
    void         (*buffer_flush)  (Buffer, u64 offset, u64 size); // explicit flush for non-coherent

    // ---- Textures ----
    Texture (*texture_create) (const TextureDesc&);
    void    (*texture_destroy)(Texture);

    // ---- Samplers ----
    Sampler (*sampler_create) (const SamplerDesc&);
    void    (*sampler_destroy)(Sampler);

    // ---- Shaders ----
    Shader (*shader_create) (const ShaderDesc&);
    void   (*shader_destroy)(Shader);

    // ---- Pipelines ----
    Pipeline (*pipeline_create) (const PipelineDesc&);
    void     (*pipeline_destroy)(Pipeline);

    // ---- Descriptor layouts ----
    DescriptorLayout (*descriptor_layout_create) (const DescriptorLayoutDesc&);
    void             (*descriptor_layout_destroy)(DescriptorLayout);

    // ---- Descriptor sets ----
    DescriptorSet (*descriptor_set_create) (DescriptorLayout);
    void          (*descriptor_set_destroy)(DescriptorSet);
    void          (*descriptor_set_write)  (DescriptorSet, const DescriptorWrite*, u32 count);

    // ---- Fences ----
    Fence (*fence_create)     (bool signaled);
    void  (*fence_destroy)    (Fence);
    bool  (*fence_wait)       (const Fence*, u32 count, bool wait_all, u64 timeout_ns);
    void  (*fence_reset)      (const Fence*, u32 count);
    bool  (*fence_is_signaled)(Fence);

    // ---- Semaphores ----
    Semaphore (*semaphore_create) ();
    void      (*semaphore_destroy)(Semaphore);

    // ---- Command buffers ----
    CmdBuf (*cmdbuf_create)  (u32 queue_family); // 0=graphics 1=compute 2=transfer
    void   (*cmdbuf_destroy) (CmdBuf);
    void   (*cmdbuf_begin)   (CmdBuf);
    void   (*cmdbuf_end)     (CmdBuf);
    void   (*cmdbuf_reset)   (CmdBuf);

    // ---- Render pass (dynamic rendering — no VkRenderPass objects) ----
    void (*cmd_begin_render_pass)(CmdBuf, const RenderPassDesc&);
    void (*cmd_end_render_pass)  (CmdBuf);

    // ---- Viewport / scissor ----
    void (*cmd_set_viewport)(CmdBuf, const Viewport&);
    void (*cmd_set_scissor) (CmdBuf, const Rect&);

    // ---- State ----
    void (*cmd_bind_pipeline)      (CmdBuf, Pipeline);
    void (*cmd_bind_descriptor_set)(CmdBuf, DescriptorSet, u32 set_index, const u32* dynamic_offsets, u32 dynamic_count);
    void (*cmd_bind_vertex_buffers)(CmdBuf, u32 first, const Buffer*, const u64* offsets, u32 count);
    void (*cmd_bind_index_buffer)  (CmdBuf, Buffer, u64 offset, IndexType);
    void (*cmd_push_constants)     (CmdBuf, ShaderStage stages, u32 offset, u32 size, const void* data);

    // ---- Draw / dispatch ----
    void (*cmd_draw)              (CmdBuf, const DrawCmd&);
    void (*cmd_draw_indexed)      (CmdBuf, const DrawIndexedCmd&);
    void (*cmd_draw_indirect)     (CmdBuf, Buffer args, u64 offset, u32 draw_count, u32 stride);
    void (*cmd_draw_indexed_indirect)(CmdBuf, Buffer args, u64 offset, u32 draw_count, u32 stride);
    void (*cmd_dispatch)          (CmdBuf, const DispatchCmd&);
    void (*cmd_dispatch_indirect) (CmdBuf, Buffer args, u64 offset);

    // ---- Transfer ----
    void (*cmd_copy_buffer)          (CmdBuf, const BufferCopy&);
    void (*cmd_copy_texture)         (CmdBuf, const TextureCopy&);
    void (*cmd_copy_buffer_to_texture)(CmdBuf, const BufferTextureCopy&);
    void (*cmd_copy_texture_to_buffer)(CmdBuf, const BufferTextureCopy&);
    void (*cmd_blit_texture)         (CmdBuf, Texture src, Texture dst,
                                       const Rect& src_rect, const Rect& dst_rect,
                                       u32 src_mip, u32 dst_mip,
                                       SamplerFilter filter);
    void (*cmd_clear_color_texture)  (CmdBuf, Texture, const ClearColor&,
                                       u32 base_mip, u32 mip_count,
                                       u32 base_layer, u32 layer_count);
    void (*cmd_clear_depth_texture)  (CmdBuf, Texture, const ClearDepth&);
    void (*cmd_fill_buffer)          (CmdBuf, Buffer, u64 offset, u64 size, u32 value);

    // ---- Barriers ----
    void (*cmd_texture_barrier) (CmdBuf, const TextureBarrier*, u32 count);
    void (*cmd_buffer_barrier)  (CmdBuf, const BufferBarrier*,  u32 count);
    void (*cmd_global_barrier)  (CmdBuf,
                                  PipelineStage src_stage, Access src_access,
                                  PipelineStage dst_stage, Access dst_access);

    // ---- Debug markers (GPU profiling / RenderDoc / PIX) ----
    void (*cmd_begin_region) (CmdBuf, const char* name, f32 r, f32 g, f32 b);
    void (*cmd_end_region)   (CmdBuf);
    void (*cmd_marker)       (CmdBuf, const char* name);

    // ---- Submit ----
    void (*queue_submit)    (const CmdBuf* cmds, u32 cmd_count,
                             const Semaphore* wait_sems,   u32 wait_count,
                             const Semaphore* signal_sems, u32 signal_count,
                             Fence signal_fence);
    void (*queue_wait_idle) (u32 queue_family);
    void (*device_wait_idle)();
};

// =============================================================
//  GLOBAL DISPATCH
// =============================================================

extern const BackendApi* g_rhi;

bool init    (const InitDesc&);
void shutdown();

// =============================================================
//  ASSERT GUARD
// =============================================================

#ifndef NDEBUG
  #define _RHI_CHECK() assert(::rhi::g_rhi && "rhi::init() not called")
#else
  #define _RHI_CHECK() ((void)0)
#endif

// =============================================================
//  FREE FUNCTION DISPATCH WRAPPERS
//  Callers use these — never call through g_rhi directly.
// =============================================================

// ---- Device ----
inline void get_device_info(DeviceInfo& out)
    { _RHI_CHECK(); g_rhi->get_device_info(out); }

// ---- Swapchain ----
[[nodiscard]] inline bool swapchain_create(const SwapchainDesc& d)
    { _RHI_CHECK(); return g_rhi->swapchain_create(d); }
inline void swapchain_destroy()
    { _RHI_CHECK(); g_rhi->swapchain_destroy(); }
[[nodiscard]] inline bool swapchain_acquire(SwapchainFrame& f, Semaphore s)
    { _RHI_CHECK(); return g_rhi->swapchain_acquire(f, s); }
[[nodiscard]] inline bool swapchain_present(Semaphore s)
    { _RHI_CHECK(); return g_rhi->swapchain_present(s); }
[[nodiscard]] inline bool swapchain_resize(u32 w, u32 h)
    { _RHI_CHECK(); return g_rhi->swapchain_resize(w, h); }

// ---- Buffers ----
[[nodiscard]] inline Buffer buffer_create(const BufferDesc& d)
    { _RHI_CHECK(); return g_rhi->buffer_create(d); }
inline void buffer_destroy(Buffer h)
    { _RHI_CHECK(); if (h) g_rhi->buffer_destroy(h); }
[[nodiscard]] inline MappedBuffer buffer_map(Buffer h)
    { _RHI_CHECK(); return g_rhi->buffer_map(h); }
inline void buffer_unmap(Buffer h)
    { _RHI_CHECK(); g_rhi->buffer_unmap(h); }
inline void buffer_flush(Buffer h, u64 offset = 0, u64 size = 0)
    { _RHI_CHECK(); g_rhi->buffer_flush(h, offset, size); }

// ---- Textures ----
[[nodiscard]] inline Texture texture_create(const TextureDesc& d)
    { _RHI_CHECK(); return g_rhi->texture_create(d); }
inline void texture_destroy(Texture h)
    { _RHI_CHECK(); if (h) g_rhi->texture_destroy(h); }

// ---- Samplers ----
[[nodiscard]] inline Sampler sampler_create(const SamplerDesc& d)
    { _RHI_CHECK(); return g_rhi->sampler_create(d); }
inline void sampler_destroy(Sampler h)
    { _RHI_CHECK(); if (h) g_rhi->sampler_destroy(h); }

// ---- Shaders ----
[[nodiscard]] inline Shader shader_create(const ShaderDesc& d)
    { _RHI_CHECK(); return g_rhi->shader_create(d); }
inline void shader_destroy(Shader h)
    { _RHI_CHECK(); if (h) g_rhi->shader_destroy(h); }

// ---- Pipelines ----
[[nodiscard]] inline Pipeline pipeline_create(const PipelineDesc& d)
    { _RHI_CHECK(); return g_rhi->pipeline_create(d); }
inline void pipeline_destroy(Pipeline h)
    { _RHI_CHECK(); if (h) g_rhi->pipeline_destroy(h); }

// ---- Descriptor layouts ----
[[nodiscard]] inline DescriptorLayout descriptor_layout_create(const DescriptorLayoutDesc& d)
    { _RHI_CHECK(); return g_rhi->descriptor_layout_create(d); }
inline void descriptor_layout_destroy(DescriptorLayout h)
    { _RHI_CHECK(); if (h) g_rhi->descriptor_layout_destroy(h); }

// ---- Descriptor sets ----
[[nodiscard]] inline DescriptorSet descriptor_set_create(DescriptorLayout l)
    { _RHI_CHECK(); return g_rhi->descriptor_set_create(l); }
inline void descriptor_set_destroy(DescriptorSet h)
    { _RHI_CHECK(); if (h) g_rhi->descriptor_set_destroy(h); }
inline void descriptor_set_write(DescriptorSet h, const DescriptorWrite* w, u32 n)
    { _RHI_CHECK(); g_rhi->descriptor_set_write(h, w, n); }

// Convenience overload for a single write
inline void descriptor_set_write(DescriptorSet h, const DescriptorWrite& w)
    { descriptor_set_write(h, &w, 1); }

// ---- Fences ----
[[nodiscard]] inline Fence fence_create(bool signaled = false)
    { _RHI_CHECK(); return g_rhi->fence_create(signaled); }
inline void fence_destroy(Fence h)
    { _RHI_CHECK(); if (h) g_rhi->fence_destroy(h); }
inline bool fence_wait(const Fence* hs, u32 n, bool all = true, u64 timeout_ns = ~0ull)
    { _RHI_CHECK(); return g_rhi->fence_wait(hs, n, all, timeout_ns); }
// Single-fence convenience
inline bool fence_wait(Fence h, u64 timeout_ns = ~0ull)
    { return fence_wait(&h, 1, true, timeout_ns); }
inline void fence_reset(const Fence* hs, u32 n)
    { _RHI_CHECK(); g_rhi->fence_reset(hs, n); }
inline void fence_reset(Fence h)
    { fence_reset(&h, 1); }
[[nodiscard]] inline bool fence_is_signaled(Fence h)
    { _RHI_CHECK(); return g_rhi->fence_is_signaled(h); }

// ---- Semaphores ----
[[nodiscard]] inline Semaphore semaphore_create()
    { _RHI_CHECK(); return g_rhi->semaphore_create(); }
inline void semaphore_destroy(Semaphore h)
    { _RHI_CHECK(); if (h) g_rhi->semaphore_destroy(h); }

// ---- Command buffers ----
[[nodiscard]] inline CmdBuf cmdbuf_create(u32 qf = 0)
    { _RHI_CHECK(); return g_rhi->cmdbuf_create(qf); }
inline void cmdbuf_destroy(CmdBuf h) { _RHI_CHECK(); g_rhi->cmdbuf_destroy(h); }
inline void cmdbuf_begin(CmdBuf h)   { _RHI_CHECK(); g_rhi->cmdbuf_begin(h); }
inline void cmdbuf_end(CmdBuf h)     { _RHI_CHECK(); g_rhi->cmdbuf_end(h); }
inline void cmdbuf_reset(CmdBuf h)   { _RHI_CHECK(); g_rhi->cmdbuf_reset(h); }

// ---- Render pass ----
inline void begin_render_pass(CmdBuf c, const RenderPassDesc& d)
    { _RHI_CHECK(); g_rhi->cmd_begin_render_pass(c, d); }
inline void end_render_pass(CmdBuf c)
    { _RHI_CHECK(); g_rhi->cmd_end_render_pass(c); }

// ---- Viewport / scissor ----
inline void set_viewport(CmdBuf c, const Viewport& vp)
    { _RHI_CHECK(); g_rhi->cmd_set_viewport(c, vp); }
inline void set_scissor(CmdBuf c, const Rect& r)
    { _RHI_CHECK(); g_rhi->cmd_set_scissor(c, r); }

// ---- State ----
inline void bind_pipeline(CmdBuf c, Pipeline p)
    { _RHI_CHECK(); g_rhi->cmd_bind_pipeline(c, p); }
inline void bind_descriptor_set(CmdBuf c, DescriptorSet s, u32 slot = 0,
                                 const u32* dyn = nullptr, u32 dyn_count = 0)
    { _RHI_CHECK(); g_rhi->cmd_bind_descriptor_set(c, s, slot, dyn, dyn_count); }
inline void bind_vertex_buffers(CmdBuf c, u32 first, const Buffer* bufs,
                                 const u64* offsets, u32 n)
    { _RHI_CHECK(); g_rhi->cmd_bind_vertex_buffers(c, first, bufs, offsets, n); }
// Single vertex buffer convenience
inline void bind_vertex_buffer(CmdBuf c, Buffer buf, u64 offset = 0)
    { bind_vertex_buffers(c, 0, &buf, &offset, 1); }
inline void bind_index_buffer(CmdBuf c, Buffer b, u64 off, IndexType t)
    { _RHI_CHECK(); g_rhi->cmd_bind_index_buffer(c, b, off, t); }
template<typename T>
inline void push_constants(CmdBuf c, const T& data,
                            ShaderStage stages = ShaderStage::All, u32 offset = 0)
    { _RHI_CHECK(); g_rhi->cmd_push_constants(c, stages, offset, sizeof(T), &data); }
inline void push_constants_raw(CmdBuf c, ShaderStage stages, u32 offset, u32 size, const void* data)
    { _RHI_CHECK(); g_rhi->cmd_push_constants(c, stages, offset, size, data); }

// ---- Draw / dispatch ----
inline void draw(CmdBuf c, const DrawCmd& d)        { _RHI_CHECK(); g_rhi->cmd_draw(c, d); }
inline void draw_indexed(CmdBuf c, const DrawIndexedCmd& d) { _RHI_CHECK(); g_rhi->cmd_draw_indexed(c, d); }
inline void draw_indirect(CmdBuf c, Buffer a, u64 off, u32 count, u32 stride)
    { _RHI_CHECK(); g_rhi->cmd_draw_indirect(c, a, off, count, stride); }
inline void draw_indexed_indirect(CmdBuf c, Buffer a, u64 off, u32 count, u32 stride)
    { _RHI_CHECK(); g_rhi->cmd_draw_indexed_indirect(c, a, off, count, stride); }
inline void dispatch(CmdBuf c, const DispatchCmd& d){ _RHI_CHECK(); g_rhi->cmd_dispatch(c, d); }
inline void dispatch(CmdBuf c, u32 x, u32 y = 1, u32 z = 1) { dispatch(c, {x, y, z}); }
inline void dispatch_indirect(CmdBuf c, Buffer a, u64 off)
    { _RHI_CHECK(); g_rhi->cmd_dispatch_indirect(c, a, off); }

// ---- Transfer ----
inline void copy_buffer(CmdBuf c, const BufferCopy& cp)
    { _RHI_CHECK(); g_rhi->cmd_copy_buffer(c, cp); }
inline void copy_texture(CmdBuf c, const TextureCopy& cp)
    { _RHI_CHECK(); g_rhi->cmd_copy_texture(c, cp); }
inline void copy_buffer_to_texture(CmdBuf c, const BufferTextureCopy& cp)
    { _RHI_CHECK(); g_rhi->cmd_copy_buffer_to_texture(c, cp); }
inline void copy_texture_to_buffer(CmdBuf c, const BufferTextureCopy& cp)
    { _RHI_CHECK(); g_rhi->cmd_copy_texture_to_buffer(c, cp); }
inline void blit_texture(CmdBuf c, Texture src, Texture dst,
                          const Rect& sr, const Rect& dr,
                          u32 sm = 0, u32 dm = 0,
                          SamplerFilter filter = SamplerFilter::Linear)
    { _RHI_CHECK(); g_rhi->cmd_blit_texture(c, src, dst, sr, dr, sm, dm, filter); }
inline void clear_color_texture(CmdBuf c, Texture t, const ClearColor& cc,
                                  u32 bm = 0, u32 nm = 0, u32 bl = 0, u32 nl = 0)
    { _RHI_CHECK(); g_rhi->cmd_clear_color_texture(c, t, cc, bm, nm, bl, nl); }
inline void clear_depth_texture(CmdBuf c, Texture t, const ClearDepth& cd)
    { _RHI_CHECK(); g_rhi->cmd_clear_depth_texture(c, t, cd); }
inline void fill_buffer(CmdBuf c, Buffer b, u64 offset, u64 size, u32 val)
    { _RHI_CHECK(); g_rhi->cmd_fill_buffer(c, b, offset, size, val); }

// ---- Barriers ----
inline void texture_barrier(CmdBuf c, const TextureBarrier* bs, u32 n)
    { _RHI_CHECK(); g_rhi->cmd_texture_barrier(c, bs, n); }
inline void texture_barrier(CmdBuf c, const TextureBarrier& b)
    { texture_barrier(c, &b, 1); }
inline void buffer_barrier(CmdBuf c, const BufferBarrier* bs, u32 n)
    { _RHI_CHECK(); g_rhi->cmd_buffer_barrier(c, bs, n); }
inline void buffer_barrier(CmdBuf c, const BufferBarrier& b)
    { buffer_barrier(c, &b, 1); }
inline void global_barrier(CmdBuf c,
                             PipelineStage src_stage, Access src_access,
                             PipelineStage dst_stage, Access dst_access)
    { _RHI_CHECK(); g_rhi->cmd_global_barrier(c, src_stage, src_access, dst_stage, dst_access); }

// ---- Debug ----
inline void begin_region(CmdBuf c, const char* n, f32 r=1.f, f32 g=1.f, f32 b=1.f)
    { _RHI_CHECK(); g_rhi->cmd_begin_region(c, n, r, g, b); }
inline void end_region(CmdBuf c)
    { _RHI_CHECK(); g_rhi->cmd_end_region(c); }
inline void marker(CmdBuf c, const char* n)
    { _RHI_CHECK(); g_rhi->cmd_marker(c, n); }

// ---- Submit ----
inline void queue_submit(const CmdBuf* cmds, u32 nc,
                          const Semaphore* wsems = nullptr, u32 nw = 0,
                          const Semaphore* ssems = nullptr, u32 ns = 0,
                          Fence fence = {})
    { _RHI_CHECK(); g_rhi->queue_submit(cmds, nc, wsems, nw, ssems, ns, fence); }
inline void queue_wait_idle(u32 qf = 0)
    { _RHI_CHECK(); g_rhi->queue_wait_idle(qf); }
inline void device_wait_idle()
    { _RHI_CHECK(); g_rhi->device_wait_idle(); }

#undef _RHI_CHECK

// =============================================================
//  FrameContext
//  Per-in-flight-frame resources for double/triple buffering.
//  Application creates N of these (N = MAX_FRAMES_IN_FLIGHT).
// =============================================================

struct FrameContext
{
    CmdBuf    cmd           = {};   // primary graphics command buffer
    Fence     fence         = {};   // CPU waits on this before reusing the frame
    Semaphore image_ready   = {};   // swapchain image acquisition signal
    Semaphore render_done   = {};   // present waits on this
    u32       index         = 0;    // 0..MAX_FRAMES_IN_FLIGHT-1
};

// =============================================================
//  RAII SCOPED WRAPPERS
//  For short-lived resources (staging buffers, upload fences).
//  Use explicit destroy() for persistent resources — clearer
//  ownership and easier to reason about lifetimes.
// =============================================================

template<typename H, void(*Dtor)(H)>
struct Scoped
{
    H h{};
    explicit Scoped(H handle) : h(handle) {}
    ~Scoped() { if (h) Dtor(h); }
    Scoped(const Scoped&)             = delete;
    Scoped& operator=(const Scoped&)  = delete;
    Scoped(Scoped&& o) noexcept : h(o.h) { o.h = {}; }
    operator H() const { return h; }
    H release() { H tmp = h; h = {}; return tmp; }
};

using ScopedBuffer    = Scoped<Buffer,    buffer_destroy>;
using ScopedTexture   = Scoped<Texture,   texture_destroy>;
using ScopedShader    = Scoped<Shader,    shader_destroy>;
using ScopedPipeline  = Scoped<Pipeline,  pipeline_destroy>;
using ScopedFence     = Scoped<Fence,     fence_destroy>;
using ScopedSemaphore = Scoped<Semaphore, semaphore_destroy>;
using ScopedCmdBuf    = Scoped<CmdBuf,    cmdbuf_destroy>;

// =============================================================
//  UploadContext
//  Immediate synchronous upload helper.  Uses a dedicated
//  transfer CmdBuf and waits on the CPU.  Good for asset
//  loading; for streaming use a ring buffer instead.
// =============================================================

struct UploadContext
{
    CmdBuf transfer_cmd = {};
    Fence  done         = {};
    Buffer staging      = {};
    void  *staging_ptr  = nullptr;
    u64    staging_size = 0;

    static UploadContext create(u64 staging_capacity)
    {
        UploadContext ctx;
        ctx.staging = buffer_create({
            .size   = staging_capacity,
            .usage  = BufferUsage::TransferSrc,
            .memory = MemoryType::CpuToGpu,
            .name   = "upload_staging",
        });
        auto m = buffer_map(ctx.staging);
        ctx.staging_ptr  = m.ptr;
        ctx.staging_size = m.size;
        ctx.transfer_cmd = cmdbuf_create(2); // transfer queue
        ctx.done         = fence_create(false);
        return ctx;
    }

    // Upload a region of data into a GPU buffer.
    // staging_offset must be tracked by the caller if batching.
    void upload_buffer(Buffer dst, const void* src, u64 size, u64 dst_offset = 0)
    {
        assert(size <= staging_size);
        memcpy(staging_ptr, src, size);
        copy_buffer(transfer_cmd, {
            .src = staging, .src_offset = 0,
            .dst = dst,     .dst_offset = dst_offset,
            .size = size,
        });
    }

    void begin() { cmdbuf_reset(transfer_cmd); cmdbuf_begin(transfer_cmd); }

    void submit_and_wait()
    {
        cmdbuf_end(transfer_cmd);
        queue_submit(&transfer_cmd, 1, nullptr, 0, nullptr, 0, done);
        fence_wait(done);
        fence_reset(done);
    }

    void destroy()
    {
        cmdbuf_destroy(transfer_cmd);
        fence_destroy(done);
        buffer_unmap(staging);
        buffer_destroy(staging);
        *this = {};
    }
};

} // namespace rhi
