#include "TriangleDemo.h"

#include <imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <GLFW/glfw3.h>

// Extern shader bytecode - expected to be linked or generated
extern const unsigned char g_triangle_vert_spv[];
extern const unsigned long long g_triangle_vert_spv_size;
extern const unsigned char g_triangle_frag_spv[];
extern const unsigned long long g_triangle_frag_spv_size;
static std::vector<char> readFile(const std::string& filename) 
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary); 

        if (!file.is_open()) 
        {
            throw std::runtime_error("failed to open file!");
        }
        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
            
        return buffer;
    }

// Simple vertex data
struct Vertex
{
    float x, y;
    float r, g, b;
};

static constexpr Vertex k_verts[3] =
{
    {  0.0f, -0.5f,   1.f, 0.0f, 0.0f },
    {  0.5f,  0.5f,   0.0f, 1.f, 0.0f },
    { -0.5f,  0.5f,   0.0f, 0.0f, 1.f },
};

TriangleDemo::TriangleDemo()
	:
	Engine::Layer("Triangle"), m_cameraController(1280.0f / 720.0f)
{
}

void TriangleDemo::OnAttach()
{
    auto& window = Engine::EngineApp::Get().GetWindow();
    GLFWwindow* nativeWindow = static_cast<GLFWwindow*>(window.GetNativeWindow());

    // 1. Init RHI
    if (!rhi::init({
        .backend = rhi::Backend::Vulkan,
        .validation = true,
        .app_name = "Usul Triangle Demo"
    }))
    {
        ENGINE_ERROR("Failed to initialize RHI");
        return;
    }

    // 2. Create Swapchain
    if (!rhi::swapchain_create({
        .window_handle = nativeWindow,
        .width = window.GetWidth(),
        .height = window.GetHeight(),
        .image_count = rhi::MAX_FRAMES_IN_FLIGHT,
        .format = rhi::Format::BGRA8_Srgb,
        .vsync = true,
        .window_type = rhi::WindowType::Glfw
    }))
    {
        ENGINE_ERROR("Failed to create swapchain");
        return;
    }

    // 3. Create Vertex Buffer
    m_vb = rhi::buffer_create({
        .size = sizeof(k_verts),
        .usage = rhi::BufferUsage::Vertex | rhi::BufferUsage::TransferDst,
        .memory = rhi::MemoryType::GpuOnly,
        .name = "TriangleVB"
    });

    // Upload data
    auto upload = rhi::UploadContext::create(sizeof(k_verts));
    upload.begin();
    upload.upload_buffer(m_vb, k_verts, sizeof(k_verts));
    upload.submit_and_wait();
    upload.destroy();

    // 4. Create Shaders
    // Note: In a real app, you would load these from files or use a shader compilation system
    auto vertShaderCode = readFile(Engine::AssetManager::GetAssetPath("shaders/vert.spv").string());
    auto fragShaderCode = readFile(Engine::AssetManager::GetAssetPath("shaders/frag.spv").string());
    
    m_vs = rhi::shader_create({
        .bytecode = reinterpret_cast<const uint32_t*>(vertShaderCode.data()),
        .size = vertShaderCode.size(),
        .stage = rhi::ShaderStage::Vertex,
        .entry = "main",
        .name = "TriangleVS"
    });

    m_fs = rhi::shader_create({
        .bytecode = reinterpret_cast<const uint32_t*>(fragShaderCode.data()),
        .size = fragShaderCode.size(),
        .stage = rhi::ShaderStage::Fragment,
        .entry = "main",
        .name = "TriangleFS"
    });

    // 5. Create Pipeline
    m_pipeline = rhi::pipeline_create({
        .vertex_shader = m_vs,
        .fragment_shader = m_fs,
        .attribs = {
            { .binding = 0, .location = 0, .format = rhi::Format::RG32_Float, .offset = offsetof(Vertex, x) },
            { .binding = 0, .location = 1, .format = rhi::Format::RGB32_Float, .offset = offsetof(Vertex, r) }
        },
        .attrib_count = 2,
        .bindings = {
            { .binding = 0, .stride = sizeof(Vertex), .input_rate = rhi::VertexInputRate::Vertex }
        },
        .binding_count = 1,
        .color_formats = { rhi::Format::BGRA8_Srgb },
        .color_count = 1,
        .topology = rhi::PrimitiveTopology::TriangleList,
        .name = "TrianglePipeline"
    });

    // 6. Create Sync Objects
    for (uint32_t i = 0; i < rhi::MAX_FRAMES_IN_FLIGHT; ++i)
    {
        m_frames[i].index = i;
        m_frames[i].cmd = rhi::cmdbuf_create(0);
        m_frames[i].fence = rhi::fence_create(true);
        m_frames[i].image_ready = rhi::semaphore_create();
        m_frames[i].render_done = rhi::semaphore_create();
    }

    m_rhi_initialized = true;
}

void TriangleDemo::OnDetach()
{
    if (m_rhi_initialized)
    {
        rhi::device_wait_idle();
        for (auto& f : m_frames)
        {
            rhi::cmdbuf_destroy(f.cmd);
            rhi::fence_destroy(f.fence);
            rhi::semaphore_destroy(f.image_ready);
            rhi::semaphore_destroy(f.render_done);
        }
        rhi::pipeline_destroy(m_pipeline);
        rhi::shader_destroy(m_vs);
        rhi::shader_destroy(m_fs);
        rhi::buffer_destroy(m_vb);
        rhi::swapchain_destroy();
        rhi::shutdown();
    }
}

void TriangleDemo::OnUpdate(const Engine::Timestep& ts)
{
    if (!m_rhi_initialized) return;

    auto& f = m_frames[m_frame_index];

    rhi::fence_wait(f.fence);
    rhi::fence_reset(f.fence);

    rhi::SwapchainFrame sw;
    if (!rhi::swapchain_acquire(sw, f.image_ready))
        return;

    rhi::cmdbuf_reset(f.cmd);
    rhi::cmdbuf_begin(f.cmd);

    rhi::texture_barrier(f.cmd, {
        .tex = sw.backbuffer,
        .old_layout = rhi::TextureLayout::Undefined,
        .new_layout = rhi::TextureLayout::ColorTarget,
        .src_stage = rhi::PipelineStage::Top,
        .dst_stage = rhi::PipelineStage::ColorOutput,
        .src_access = rhi::Access::None,
        .dst_access = rhi::Access::ColorWrite
    });

    rhi::begin_render_pass(f.cmd, {
        .color = {{
            .texture = sw.backbuffer,
            .load_op = rhi::LoadOp::Clear,
            .store_op = rhi::StoreOp::Store,
            .clear = { .r = 0.1f, .g = 0.1f, .b = 0.1f, .a = 1.0f }
        }},
        .color_count = 1
    });

    auto& window = Engine::EngineApp::Get().GetWindow();
    float width = (float)window.GetWidth();
    float height = (float)window.GetHeight();

    rhi::set_viewport(f.cmd, { .x = 0, .y = 0, .w = width, .h = height, .min_depth = 0, .max_depth = 1 });
    rhi::set_scissor(f.cmd, { .x = 0, .y = 0, .w = (uint32_t)width, .h = (uint32_t)height });

    rhi::bind_pipeline(f.cmd, m_pipeline);
    rhi::bind_vertex_buffer(f.cmd, m_vb);
    rhi::draw(f.cmd, { .vertex_count = 3 });

    rhi::end_render_pass(f.cmd);

    rhi::texture_barrier(f.cmd, {
        .tex = sw.backbuffer,
        .old_layout = rhi::TextureLayout::ColorTarget,
        .new_layout = rhi::TextureLayout::Present,
        .src_stage = rhi::PipelineStage::ColorOutput,
        .dst_stage = rhi::PipelineStage::Bottom,
        .src_access = rhi::Access::ColorWrite,
        .dst_access = rhi::Access::None
    });

    rhi::cmdbuf_end(f.cmd);

    rhi::queue_submit(&f.cmd, 1, &f.image_ready, 1, &f.render_done, 1, f.fence);
    rhi::swapchain_present(f.render_done);

    m_frame_index = (m_frame_index + 1) % rhi::MAX_FRAMES_IN_FLIGHT;
}

void TriangleDemo::OnImGuiRender()
{
}

void TriangleDemo::OnEvent(Engine::Event& e)
{
}
