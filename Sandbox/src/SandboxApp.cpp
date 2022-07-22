#include <Usul.h>
#include "imgui/imgui.h"

class ExampleLayer : public Usul::Layer
{
public:
	ExampleLayer()
		:Layer("Example")
	{

	}

	

	void OnAttach() override
	{
		
	}

	virtual void OnImGuiRender() override
	{
		ImGui::Begin("Test");
		ImGui::Text("Hello World");
		ImGui::End();
	}

	void OnEvent(Usul::Event & event) override
	{
		if (event.GetEventType() == Usul::EventType::KeyPressed)
		{
			Usul::KeyPressedEvent& e = (Usul::KeyPressedEvent&)event;
			if (e.GetKeyCode() == US_KEY_TAB)
				US_TRACE("Tab key is pressed (event)!");
			US_TRACE("{0}", (char)e.GetKeyCode());
		}
	}

};

class Sandbox : public Usul::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
	}
	~Sandbox()
	{
	}
};

Usul::Application* Usul::CreateApplication()
{
	return new Sandbox();
}