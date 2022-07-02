#pragma once

#include "Window.h"
#include "Core.h"
#include "LayerStack.h"

namespace Usul
{
	class WindowCloseEvent;

	class USUL_API Application
	{
		public:
			Application();
			virtual ~Application();

			void OnEvent(Event& e);
			void Run();
			bool OnWindowClose(WindowCloseEvent &e);
			void PushLayer(Layer* layer);
			void PushOverLay(Layer* layer);
	private:
		std::unique_ptr<Window> m_Window;
		bool m_Running = true;

		LayerStack m_LayerStack;
	};

	//TO be defined in client
	Application * CreateApplication();
}

