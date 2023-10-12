#pragma once

#ifdef US_PLATFORM_WINDOWS

extern Usul::Application* CreateApplication();

int main(int argc, char** argv)
{
	Usul::Log::Init();
	Usul::Log::GetCoreLogger()->warn("init log");
	auto app = Usul::CreateApplication();
	app->Run();
	delete app;
}

#endif // US_PLATFORM_WINDOWS
