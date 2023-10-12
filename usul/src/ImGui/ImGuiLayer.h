#pragma once
#include "usul/Layer.h"

namespace Usul
{
	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();
		void OnUpdate() override;
		void OnEvent(Event& event) override;

		void OnAttach() override;


		void OnDetach() override;

		virtual void OnImGuiRender() override;

		void Begin();
		void End();
	private:
		float m_time = 0.0f;
	};
}


