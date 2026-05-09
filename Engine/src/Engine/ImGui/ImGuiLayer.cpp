#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "ImGuiLayer.h"
#include "imgui.h"

#include "Engine/Core/EngineApp.h"

#include "Event/ApplicationEvent.h"
#include "ImGuizmo.h"
#include "RHI/rhi.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// #define ENGINE_BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

namespace Engine
{
static ImGuiDockNodeFlags dockspace_flags =
    ImGuiDockNodeFlags_PassthruCentralNode;

// RHI Resources
static rhi::Texture s_ColorTex = {};
static rhi::Texture s_DepthTex = {};
static rhi::CmdBuf s_Cmd = {};
static rhi::Fence s_Fence = {};
static uint32_t s_ViewportWidth = 1280;
static uint32_t s_ViewportHeight = 720;

// Per-frame present resources
struct PresentFrame {
    rhi::CmdBuf   cmd          = {};
    rhi::Fence    fence        = {};
    rhi::Semaphore image_ready = {};
    rhi::Semaphore render_done = {};
};
static PresentFrame s_Frames[rhi::MAX_FRAMES_IN_FLIGHT];
static uint32_t     s_FrameIndex = 0;

ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer") {}

ImGuiLayer::~ImGuiLayer() {}

void ImGuiLayer::OnAttach()
{
#if ENABLE_EDITOR_MODE
  s_ViewportWidth = 1280;
  s_ViewportHeight = 720;

  s_ColorTex = rhi::texture_create(
      {.width = s_ViewportWidth,
       .height = s_ViewportHeight,
       .format = rhi::Format::BGRA8_Srgb,
       .usage = rhi::TextureUsage::ColorTarget | rhi::TextureUsage::Sampled,
       .name = "ViewportColor"});

  s_DepthTex = rhi::texture_create({.width = s_ViewportWidth,
                                    .height = s_ViewportHeight,
                                    .format = rhi::Format::D32_Float,
                                    .usage = rhi::TextureUsage::DepthTarget,
                                    .name = "ViewportDepth"});

  s_Cmd = rhi::cmdbuf_create(0);
  s_Fence = rhi::fence_create(false);
#endif

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable EventCategoryKeyboard
                                          // Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport /
                                                      // Platform Windows

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsClassic();

  // Setup Platform/Renderer bindings
  EngineApp &app = EngineApp::Get();
  GLFWwindow *window = static_cast<GLFWwindow *>(app.GetWindow().GetWindow());

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform
  // windows can look identical to regular ones.
  ImGuiStyle &style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
  {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  // Setup Platform/Renderer bindings
  rhi::imgui_init();

  // Create per-frame present resources (always, regardless of editor mode)
  for (uint32_t i = 0; i < rhi::MAX_FRAMES_IN_FLIGHT; ++i)
  {
    s_Frames[i].cmd          = rhi::cmdbuf_create(0);
    s_Frames[i].fence        = rhi::fence_create(true);
    s_Frames[i].image_ready  = rhi::semaphore_create();
    s_Frames[i].render_done  = rhi::semaphore_create();
  }
}

void ImGuiLayer::OnDetach()
{
  // wait for GPU to finish all the operations before destroying the resources
  rhi::device_wait_idle();
  rhi::imgui_shutdown();
  ImGui::DestroyContext();
#if ENABLE_EDITOR_MODE
  rhi::texture_destroy(s_ColorTex);
  rhi::texture_destroy(s_DepthTex);
  rhi::cmdbuf_destroy(s_Cmd);
  rhi::fence_destroy(s_Fence);
#endif

  for (uint32_t i = 0; i < rhi::MAX_FRAMES_IN_FLIGHT; ++i)
  {
    rhi::cmdbuf_destroy(s_Frames[i].cmd);
    rhi::fence_destroy(s_Frames[i].fence);
    rhi::semaphore_destroy(s_Frames[i].image_ready);
    rhi::semaphore_destroy(s_Frames[i].render_done);
  }
}

void ImGuiLayer::OnImGuiRender()
{
#if ENABLE_EXAMPLE
  if (_showExamples)
  {
    ShowExamples();
  }
#endif
}

void ImGuiLayer::Begin()
{
  // Start the Dear ImGui frame
  rhi::imgui_new_frame();
  ImGui::NewFrame();

  ImGuizmo::BeginFrame();

#if ENABLE_EXAMPLE
  ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
  ImGui::Begin("Viewport");
  auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
  auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
  auto viewportOffset = ImGui::GetWindowPos();
  m_ViewportBounds[0] = {viewportMinRegion.x + viewportOffset.x,
                         viewportMinRegion.y + viewportOffset.y};
  m_ViewportBounds[1] = {viewportMaxRegion.x + viewportOffset.x,
                         viewportMaxRegion.y + viewportOffset.y};

  m_ViewportFocused = ImGui::IsWindowFocused();
  m_ViewportHovered = ImGui::IsWindowHovered();

  BlockEvents(!m_ViewportHovered);

  ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
  m_ViewportSize = {viewportPanelSize.x, viewportPanelSize.y};

  // Note: Backend maps rhi::Texture to VkDescriptorSet via imgui_add_texture.
  ImGui::Image((ImTextureID)rhi::imgui_add_texture(s_ColorTex),
               ImVec2{m_ViewportSize.x, m_ViewportSize.y}, ImVec2{0, 1},
               ImVec2{1, 0});

  ImGui::End();
  ImGui::PopStyleVar();
#endif
}

void ImGuiLayer::End()
{
  ImGuiIO &io = ImGui::GetIO();
  EngineApp &app = EngineApp::Get();

  ImGui::Render();

  // Required when ViewportsEnable is set — must call every frame after Render()
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
  {
    GLFWwindow* backup = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup);
  }

  // --- Present ImGui to the swapchain ---
  auto& f = s_Frames[s_FrameIndex];
  rhi::fence_wait(f.fence);
  rhi::fence_reset(f.fence);

  rhi::SwapchainFrame sw;
  if (!rhi::swapchain_acquire(sw, f.image_ready))
    return;

  rhi::cmdbuf_reset(f.cmd);
  rhi::cmdbuf_begin(f.cmd);

  // Transition backbuffer to color attachment
  rhi::texture_barrier(f.cmd, {
      .tex        = sw.backbuffer,
      .old_layout = rhi::TextureLayout::Undefined,
      .new_layout = rhi::TextureLayout::ColorTarget,
      .src_stage  = rhi::PipelineStage::Top,
      .dst_stage  = rhi::PipelineStage::ColorOutput,
      .src_access = rhi::Access::None,
      .dst_access = rhi::Access::ColorWrite
  });

  rhi::begin_render_pass(f.cmd, {
      .color = {{
          .texture   = sw.backbuffer,
          .load_op   = rhi::LoadOp::Clear,
          .store_op  = rhi::StoreOp::Store,
          .clear     = { .r = 0.1f, .g = 0.1f, .b = 0.1f, .a = 1.0f }
      }},
      .color_count = 1
  });

  rhi::imgui_render(f.cmd);

  rhi::end_render_pass(f.cmd);

  // Transition backbuffer to presentable
  rhi::texture_barrier(f.cmd, {
      .tex        = sw.backbuffer,
      .old_layout = rhi::TextureLayout::ColorTarget,
      .new_layout = rhi::TextureLayout::Present,
      .src_stage  = rhi::PipelineStage::ColorOutput,
      .dst_stage  = rhi::PipelineStage::Bottom,
      .src_access = rhi::Access::ColorWrite,
      .dst_access = rhi::Access::None
  });

  rhi::cmdbuf_end(f.cmd);
  rhi::queue_submit(&f.cmd, 1, &f.image_ready, 1, &f.render_done, 1, f.fence);
  rhi::swapchain_present(f.render_done);

  s_FrameIndex = (s_FrameIndex + 1) % rhi::MAX_FRAMES_IN_FLIGHT;
}

void ImGuiLayer::OnUpdate(const Timestep &ts) {}

void ImGuiLayer::OnEvent(Event &e)
{
#if ENABLE_EDITOR_MODE
  // resize
  WindowResizeEvent *we = dynamic_cast<WindowResizeEvent *>(&e);
  if (we)
  {
    uint32_t width = (uint32_t)we->GetWidth();
    uint32_t height = (uint32_t)we->GetHeight();
    if (width > 0 && height > 0 &&
        (width != s_ViewportWidth || height != s_ViewportHeight))
    {
      s_ViewportWidth = width;
      s_ViewportHeight = height;

      /*When you closed the engine or resized the window, ImGuiLayer::OnDetach()
      and ImGuiLayer::OnEvent() were immediately destroying Vulkan resources
      (internal vertex/index buffers and swapchain textures). However, the GPU
      was still executing the final frames and required those buffers.
      vkDestroyBuffer cannot be called on objects actively referenced by an
      executing command buffer. Fix: Added rhi::device_wait_idle(); before
      resource destruction inside ImGuiLayer::OnDetach() and the window resize
      block in ImGuiLayer::OnEvent(). This blocks the CPU safely until the GPU
      command buffer finishes its pending operations.*/

      rhi::device_wait_idle();
      rhi::texture_destroy(s_ColorTex);
      rhi::texture_destroy(s_DepthTex);

      s_ColorTex = rhi::texture_create(
          {.width = s_ViewportWidth,
           .height = s_ViewportHeight,
           .format = rhi::Format::BGRA8_Srgb,
           .usage = rhi::TextureUsage::ColorTarget | rhi::TextureUsage::Sampled,
           .name = "ViewportColor"});

      s_DepthTex = rhi::texture_create({.width = s_ViewportWidth,
                                        .height = s_ViewportHeight,
                                        .format = rhi::Format::D32_Float,
                                        .usage = rhi::TextureUsage::DepthTarget,
                                        .name = "ViewportDepth"});
    }
    // ENGINE_WARN("{0}",we->ToString());
  }
#endif
  if (m_BlockEvents)
  {
    ImGuiIO &io = ImGui::GetIO();
    e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
    e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
  }
}


#if ENABLE_EDITOR_MODE
void ImGuiLayer::BindOrUnbindFrameBuffer(bool val)
{
  if (val)
  {
    rhi::cmdbuf_reset(s_Cmd);
    rhi::cmdbuf_begin(s_Cmd);

    rhi::TextureBarrier barriers[2] = {
        {
            .tex = s_ColorTex,
            .old_layout = rhi::TextureLayout::Undefined, .new_layout = rhi::TextureLayout::ColorTarget,
            .src_stage = rhi::PipelineStage::Top, .dst_stage = rhi::PipelineStage::ColorOutput,
            .src_access = rhi::Access::None, .dst_access = rhi::Access::ColorWrite,
            .base_mip = 0, .mip_count = 1, .base_layer = 0, .layer_count = 1
        },
        {
            .tex = s_DepthTex,
            .old_layout = rhi::TextureLayout::Undefined, .new_layout = rhi::TextureLayout::DepthStencilTarget,
            .src_stage = rhi::PipelineStage::Top, .dst_stage = rhi::PipelineStage::EarlyDepth | rhi::PipelineStage::LateDepth,
            .src_access = rhi::Access::None, .dst_access = rhi::Access::DepthWrite,
            .base_mip = 0, .mip_count = 1, .base_layer = 0, .layer_count = 1
        }
    };
    rhi::texture_barrier(s_Cmd, barriers, 2);

    rhi::begin_render_pass(s_Cmd,
                           {.color = {{.texture = s_ColorTex,
                                       .load_op = rhi::LoadOp::Clear,
                                       .store_op = rhi::StoreOp::Store,
                                       .clear = {0.1f, 0.1f, 0.1f, 1.0f}}},
                            .color_count = 1,
                            .depth = {.texture = s_DepthTex,
                                      .load_op = rhi::LoadOp::Clear,
                                      .store_op = rhi::StoreOp::Store,
                                      .clear = {1.0f, 0}},
                            .has_depth = true});
  } else
  {
    rhi::end_render_pass(s_Cmd);

    rhi::TextureBarrier resolveBar = {
        .tex = s_ColorTex,
        .old_layout = rhi::TextureLayout::ColorTarget, .new_layout = rhi::TextureLayout::ShaderReadOnly,
        .src_stage = rhi::PipelineStage::ColorOutput, .dst_stage = rhi::PipelineStage::Fragment,
        .src_access = rhi::Access::ColorWrite, .dst_access = rhi::Access::ShaderRead,
        .base_mip = 0, .mip_count = 1, .base_layer = 0, .layer_count = 1
    };
    rhi::texture_barrier(s_Cmd, &resolveBar, 1);

    rhi::cmdbuf_end(s_Cmd);
    rhi::queue_submit(&s_Cmd, 1, nullptr, 0, nullptr, 0, s_Fence);
    rhi::fence_wait(s_Fence);
    rhi::fence_reset(s_Fence);
  }
}
#endif // ENABLE_EDITOR_MODE

bool ImGuiLayer::IsViewportFocused() const { return m_ViewportFocused; }

#if ENABLE_EXAMPLE
void ImGuiLayer::AddExample(const std::string &name)
{
  _Examples.push_back(name.data());
}

void ImGuiLayer::ShowExamples() 
{
  if (_Examples.empty()) 
  {
    return;
  }
  ImGui::Begin("Example Switcher");

  // ImGui combo box to switch between objects
  if (ImGui::BeginCombo("Select Example", _Examples[_currentExampleIndex])) 
  {
    for (int i = 0; i < _Examples.size(); ++i)
    {
      bool is_selected = (_Examples[_currentExampleIndex] == _Examples[i]);
      if (ImGui::Selectable(_Examples[i], is_selected))
        _currentExampleIndex = i;
      if (is_selected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  // End the ImGui window
  ImGui::End();
}

const char *ImGuiLayer::GetCurrentExampleName()
{
  return _Examples[_currentExampleIndex];
}

#endif // ENABLE_EXAMPLE

} // namespace Engine
