#ifndef __INPUT_H__
#define __INPUT_H__

#include "Engine/Event/KeyCodes.h"
#include "Engine/Event/MouseCodes.h"
#include <glm/glm.hpp>
namespace Engine
{
    class Input
    {
    public:
        static bool IsKeyPressed(KeyCode key);

        static bool IsMouseButtonPressed(MouseCode button);
        static glm::vec2 GetMousePosition();
        static float GetMouseX();
        static float GetMouseY();
    };
}

#endif