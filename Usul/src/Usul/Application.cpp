#include "uspch.h"
#include "Application.h"
#include "Usul\Events\ApplicationEvent.h"
#include "Events/Event.h"
#include "Layer.h"
#include <glad/glad.h>
#include "Input.h"

namespace Usul
{
	Application* Application::s_Instance = nullptr;
	Application::Application()
	{
		US_CORE_ASSERT(!s_Instance, "Application already exists!")	
		s_Instance = this;

		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(BIND_EVENT_CALLBK_FN(Application::OnEvent));
	}

	Application::~Application()
	{
	}
	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_CALLBK_FN(Application::OnWindowClose));

		US_CORE_TRACE("{0}", e);

		for (auto it = m_LayerStack.end(); it!=m_LayerStack.begin();)
		{
			(*--it)->OnEvent(e);
			if (e.Handled)
			{
				break;
			}
		}
	}
	void Application::Run()
	{
		while (m_Running)
		{
			glClearColor(1, 0, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate();
			}

			auto [x, y] = Input::GetMousePosition();
			US_CORE_TRACE("{ 0 }, {1}", x, y);
			m_Window->OnUpdate();
		}
	}
	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}
	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
	}
	void Application::PushOverLay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
	}
}

