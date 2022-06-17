#include <Usul.h>

class Sandbox : public Usul::Application
{
public:
	Sandbox()
	{

	}
	~Sandbox()
	{
	}
};

Usul::Application* Usul::CreateApplication()
{
	return new Sandbox();
}