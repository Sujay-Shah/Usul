#pragma once

#include "Usul/Core.h"

namespace Usul
{
	class USUL_API Input
	{
	public:
		static bool IsKeyPressed(int keycode)
		{
			return s_Instance->IsKeyPressedImpl(keycode);
		}

		inline static bool IsMouseButtonPressed(int button) { return s_Instance->IsMouseButtonPressedImpl(button); }

		inline static float GetMouseX() { return s_Instance->GetMouseXImpl(); }
		inline static float GetMouseY() { return s_Instance->GetMouseXImpl(); }
		
		inline static std::pair<float, float> GetMousePosition() { return s_Instance->GetMousePositionImpl();  }
	protected:
		virtual bool IsKeyPressedImpl(int keyCode) = 0;

		virtual std::pair<float,float>GetMousePositionImpl() = 0;
		virtual bool IsMouseButtonPressedImpl(int button) = 0;
		virtual float GetMouseXImpl() = 0;
		virtual float GetMouseYImpl() = 0;
	
	private:
		static Input* s_Instance;
	};
}