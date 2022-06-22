#pragma once


namespace Usul
{
	class __declspec(dllexport) Application
	{
		public:
			Application();
			virtual ~Application();

			void Run();
	};

	//TO be defined in client
	Application * CreateApplication();
}

