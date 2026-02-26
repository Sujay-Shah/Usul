#ifndef __SANDBOX2D_H__
#define __SANDBOX2D_H__

#include <Engine.h>

#include "RHI/rhi.hpp"

class TriangleDemo : public Engine::Layer
{
	public:
		TriangleDemo();
		virtual ~TriangleDemo() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnUpdate(const Engine::Timestep& ts) override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(Engine::Event& e) override;
	private:
		Engine::CameraController m_cameraController;

		rhi::Buffer   m_vb       = {};
		rhi::Shader   m_vs       = {};
		rhi::Shader   m_fs       = {};
		rhi::Pipeline m_pipeline = {};

		// Per-frame-in-flight resources
		rhi::FrameContext m_frames[rhi::MAX_FRAMES_IN_FLIGHT] = {};
		uint32_t m_frame_index = 0;
		bool m_rhi_initialized = false;
};

#endif
