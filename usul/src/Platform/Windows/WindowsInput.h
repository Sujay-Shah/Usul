#pragma once
#include "usul/Input.h"

namespace Usul
{
	class WindowsInput : public Input
	{
	protected:
		bool IsKeyPressedImpl(int keycode) override;

		float GetMouseXImpl() override;


		float GetMouseYImpl() override;


		bool IsMouseButtonPressedImpl(int button) override;


		std::pair<float, float> GetMousePositionImpl() override;

	};
}


