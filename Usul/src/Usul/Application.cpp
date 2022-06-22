#include "uspch.h"
#include "Application.h"
#include "Usul/Events/ApplicationEvent.h"
#include "Usul/Log.h"

namespace Usul
{
	Application::Application()
	{
	}

	Application::~Application()
	{
	}
	void Application::Run()
	{
		WindowResizeEvent e(1280, 720);
		if (e.IsInCategory(EventCategoryApplication))
		{
			US_TRACE(e);
		}
		if (e.IsInCategory(EventCategoryInput))
		{
			US_TRACE(e);
		}
		while (true)
		{

		}
	}
}

