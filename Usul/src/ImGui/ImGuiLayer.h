#pragma once
#include "Usul/Layer.h"

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

	private:

	};
}


