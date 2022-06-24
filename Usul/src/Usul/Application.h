#pragma once

#include "Window.h"
#include "Core.h"

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
	private:
		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
	};

	//TO be defined in client
	Application * CreateApplication();
}

