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

		inline static Application& Get() { return *s_Instance; }
		inline Window& GetWindow() { return *m_Window; }

		void OnEvent(Event& e);
		void Run();
		bool OnWindowClose(WindowCloseEvent& e);
		void PushLayer(Layer* layer);
		void PushOverLay(Layer* layer);
	private:
		std::unique_ptr<Window> m_Window;
		bool m_Running = true;

		static Application* s_Instance;

		LayerStack m_LayerStack;
	};

	//TO be defined in client
	Application * CreateApplication();
}

